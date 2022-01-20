// sleep impl, using sys_sleep syscall
// in xv6, 1 tick is closed to 100ms

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int i;

    if (argc < 2 || argc > 2) {
        fprintf(2, "Usage: sleep for argv[1] ticks...\n");
        exit(1);
    }

    i = atoi(argv[1]); // ticks to sleep

    if (sleep(i) < 0) {
        fprintf(2, "sleep: failed to sleep for %s ticks\n", argv[1]);
    }

    exit(0);
}