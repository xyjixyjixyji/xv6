// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct run* steal();

#define LEN_LOCKNAME  16
struct kmem {
  struct spinlock lock;
  char lockname[LEN_LOCKNAME];
  struct run *freelist;
};

struct kmem kmems[NCPU]; /* per-CPU freelist */

void
kinit()
{
  int i;

  // name allocation
  for(i = 0; i < NCPU; ++i) {
    // first initialize the space, then the name
    snprintf(kmems[i].lockname, LEN_LOCKNAME, "kmem_%d\0", i);
  }

  // lock init: 
  // name format: kmem_0, kmem_2, ..., kmem_$(NCPU - 1)
  for(i = 0; i < NCPU; ++i) {
    initlock(&kmems[i].lock, kmems[i].lockname);
  }

  // freerange calls kfree(), giving free pages to freelist
  // however, only current running (i.e. one) cpu can get free mem
  // therefore, if multiple cores are started, the second core has no free mem
  // thats before **steal()** if implemented
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int hartid;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);
  
  // acquire current cpu #
  push_off(); // disable interrupt
  hartid = cpuid();
  pop_off();  // cpuid acquired, enable interrupt

  r = (struct run*)pa;

  acquire(&kmems[hartid].lock);
  r->next = kmems[hartid].freelist;
  kmems[hartid].freelist = r;
  release(&kmems[hartid].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  int hartid;

  // acquire current cpu #
  push_off(); // disable interrupt
  hartid = cpuid();
  pop_off();  // cpuid acquired, enable interrupt

  acquire(&kmems[hartid].lock);
  r = kmems[hartid].freelist;
  if(r)
    kmems[hartid].freelist = r->next;
  else {
    // we have no freepage
    // we steal it and returns
    r = steal(hartid);
  }
  release(&kmems[hartid].lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// @usage: steal free mem from others
// @param: hart: caller's hartid
struct run*
steal(int hartid)
{
  /** 
    * reason why caller's hart is leaked to the interface:
    *   when steal() is called, lets say called by hartid 1
    *   if the thread is scheduled to **another CPU**, the running hart of *steal()*
    *   is not necessarily the original one, this is because of the time interval
    *   between steal() is called and cpuid() is actually called
    */
  struct run *r;
  int i;

  // push_off();
  // hartid = cpuid();
  // pop_off();

  for(i = 0; i < NCPU; i++) {
    if (i == hartid)
      continue;

    acquire(&kmems[i].lock);
    if (kmems[i].freelist) {
      r = kmems[i].freelist;
      kmems[i].freelist = r->next;
      release(&kmems[i].lock);
      return r;
    }
    release(&kmems[i].lock);
  }
  return 0;
}