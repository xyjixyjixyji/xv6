// concurrently but in order, print primes under 35

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define UPPERBOUND 35

void prime(int* p)
{
    // read the first number from left, have to be prime
    int i; 

    close(p[1]);

    if (read(p[0], &i, sizeof(int)) == 0) {
        close(p[0]);
        exit(0);
    }

    printf("prime %d\n", i);

    int p2[2];
    int num;

    if (pipe(p2) < 0) {
        fprintf(2, "failed to allocate pipe\n");
    }

    if (fork() == 0) {
        // child
        close(p[0]);
        close(p2[1]);
        prime(p2);
    }
    else {
        // parent
        // read the following number, pass some to the rhs
        close(p2[0]);

        while (read(p[0], &num, sizeof(int))) {
            if (num % i != 0) {
                write(p2[1], &num, sizeof(int));
            }
        } 

        close(p2[1]);
        close(p[0]);

        wait(0); // wait for child to exit, in case the main process exits early
    }

    exit(0);
}

int
main(int argc ,char *argv[])
{
    if (argc != 1) {
        fprintf(2, "primes: no arguments needed...\n");
        exit(1);
    }

    int i;
    
    int p[2];
    pipe(p);

    if (fork() == 0) {
        prime(p);
    }
    else {
        close(p[0]);
        for (i = 2; i <= UPPERBOUND; ++i) {
            write(p[1], &i, sizeof(int));
        }
        close(p[1]);
        wait(0);
    }

    exit(0);
}