#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

// read from stdin, passed it as new args

#define MAX_ARGLENGTH 512
#define STDIN         0

int
spacecnt(char* s) // the # of args is spacecnt + 1
{
    int i = 0;
    while (*s != 0 && *s == ' ') {
        s++;
        i++;
    }
    return i;
}

int
main(int argc, char *argv[])
{
    int i, len, idx;
    char buf[MAX_ARGLENGTH], *xargv[MAXARG];

    for (i = 1; i < argc; ++i) {
        xargv[i - 1] = argv[i]; // original args
    }

    while(1) {
        idx = -1;
        do {
            idx++;
            if (idx >= MAX_ARGLENGTH) { // arguments too long...
                fprintf(2, "xargs: exceeds MAX_ARGLENGTH (default 512)...\n");
                exit(1);
            }
            len = read(STDIN, &buf[idx], 1);
        } while(len > 0 && buf[idx] != '\n'); // read the arguments until \n

        if (len == 0) break; // nothing to read anymore

        buf[idx] = 0; // end the arguments

        if ((spacecnt(buf) + 1) + argc - 1 > MAXARG) { // too many arguments...
            fprintf(2, "xargs: exceeds MAXARF (default 32)...\n");
            exit(1);
        }

        xargv[argc - 1] = buf; // concat the new arguments to the original ones

        if (fork() == 0) {
            exec(xargv[0], xargv);
        }
        else {
            wait(0);
        }
    }

    exit(0);
}