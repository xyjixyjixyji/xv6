#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc > 1) {
        fprintf(2, "uptime: no args needed...\n");
        exit(1);
    }

    printf("uptime: %d ticks\n", uptime());

    exit(0);
}