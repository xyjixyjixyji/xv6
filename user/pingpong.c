// simple impl of pingpong 

/* 
 * Write a program that uses UNIX system calls to ''ping-pong'' 
 * a byte between two processes over a pair of pipes, one for each direction. 
 * The parent should send a byte to the child; 
 * the child should print "<pid>: received ping", 
 * where <pid> is its process ID, write the byte on the pipe to the parent, and exit; 
 * the parent should read the byte from the child, print "<pid>: received pong", and exit.
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int p[2];

    pipe(p); // create pipe, p[0]: rd, p[1]: wrt

    if (fork() == 0) {
        // child process
        char *byte;
        byte = (char*) malloc(1);

        if (read(p[0], byte, 1) > 0) {
            printf("%d: received ping\n", getpid());
        }
        close(p[0]);
        write(p[1], &byte, 1);
        close(p[1]);
        exit(0);
    }
    else {
        // parent process
        char *byte;
        byte = (char*) malloc(1);

        write(p[1], byte, 1);
        close(p[1]);
        if (read(p[0], byte, 1) > 0) {
            printf("%d: received pong\n", getpid());
        }
        close(p[0]);
        exit(0);
    }

    exit(0);
}