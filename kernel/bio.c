// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

// buffer cache bucket
#define HASH(x) (x % NBUCKET)
struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  struct buf bkt[NBUCKET]; // each bucket has a doubly-linked list
  struct spinlock bktlk[NBUCKET];

} bcache;

void
binit(void)
{
  struct buf *b;
  int i;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  for(i = 0; i < NBUCKET; i++){
    initlock(&bcache.bktlk[i], "bcache_bkt");
    bcache.bkt[i].next = &bcache.bkt[i];
    bcache.bkt[i].prev = &bcache.bkt[i];
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    // hang all to the first bucket
    b->next = bcache.bkt[0].next;
    b->prev = &bcache.bkt[0];
    bcache.bkt[0].next->prev = b;
    bcache.bkt[0].next = b;
    b->ts = ticks;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bktno = HASH(blockno), i;

  // ---------- FIRST TIME CHECK ---------- //
  // Is the block already cached?
  acquire(&bcache.bktlk[bktno]);
  for(b = bcache.bkt[bktno].next; b != &bcache.bkt[bktno]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bktlk[bktno]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bktlk[bktno]);
  // ---------- FIRST TIME CHECK ---------- //

  // ---------- EVICTION ---------- //
  // evict a block, global lock used to serialize the eviction
  acquire(&bcache.lock);

  // WHY DOUBLE CHECK?
  // If two threads come here simutaneously, one add the block to cache already
  // but here, another thread will add the same block to cache again
  // RESULT: one block, cached by two buffer cache blocks 
  acquire(&bcache.bktlk[bktno]);
  for(b = bcache.bkt[bktno].next; b != &bcache.bkt[bktno]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bktlk[bktno]);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bktlk[bktno]);

  // ---------- acutal Eviction ---------- // 
  /**
   * lo: lowest buffer slot
   * lo_ts: lowest timestamp
   * lo_bkt: lowest slot's bucket 
   */
  struct buf *lo = 0;
  uint lo_ts = ~0;
  int lo_bkt = -1;

  // traverse all buckets, globally find the lowest timestamp block
  for(i = 0; i < NBUCKET; i++){
    acquire(&bcache.bktlk[i]);
    int found = 0;
    for(b = bcache.bkt[i].next; b != &bcache.bkt[i]; b = b->next){
      if((b->refcnt == 0) && (b->ts < lo_ts)) {
        if(lo){
          // this round, we found a new one, release the old lock
          lo_bkt = HASH(lo->blockno);
          if(lo_bkt != i)
            release(&bcache.bktlk[lo_bkt]);
        }
        lo_ts = b->ts;
        lo = b;
        found = 1;
      }
    }

    // At this line, we still hold one lock --> current lo_bkt's lock

    if(!found)
      release(&bcache.bktlk[i]);
    // if found, meaning that lo_bkt = i
    // which will be release when the next lo is found or the loop is over
  }

  if(!lo)
    panic("bget: no buffers");
  
  // detach it from the old bucket list
  if (HASH(lo->blockno) != bktno){
    lo->next->prev = lo->prev;
    lo->prev->next = lo->next;
  }

  // we have done the modification of the old bucket list
  release(&bcache.bktlk[HASH(lo->blockno)]);

  // attach it to the new bucket list
  if(HASH(lo->blockno) != bktno){
    acquire(&bcache.bktlk[bktno]);
    lo->next = bcache.bkt[bktno].next;
    lo->prev = &bcache.bkt[bktno];
    bcache.bkt[bktno].next->prev = lo;
    bcache.bkt[bktno].next = lo;
    release(&bcache.bktlk[bktno]);
  }

  // update at last, since older blockno was still in use
  lo->dev = dev;
  lo->blockno = blockno;
  lo->valid = 0;
  lo->refcnt = 1;
  lo->ts = ticks;

  release(&bcache.lock);
  acquiresleep(&lo->lock);

  return lo;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.bktlk[HASH(b->blockno)]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // it is evicted, so we want to use this block asap
    // the bget() will choose this block since it has a smaller ts and zero refcnt
    b->ts = ticks;
  }
  release(&bcache.bktlk[HASH(b->blockno)]);
}

void
bpin(struct buf *b) {
  acquire(&bcache.bktlk[HASH(b->blockno)]);
  b->refcnt++;
  release(&bcache.bktlk[HASH(b->blockno)]);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.bktlk[HASH(b->blockno)]);
  b->refcnt--;
  release(&bcache.bktlk[HASH(b->blockno)]);
}


