#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

void
find(char *path, char* filename) // find . b
{
    struct stat st;
    struct dirent de;
    char buf[512], *p;
    int fd;

    if ((fd = open(path, O_RDONLY)) < 0) {
        fprintf(2, "find: cannot open %s \n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s \n", path);
        close(fd);
        return;
    }

    switch (st.type) {
        case T_FILE:
            fprintf(2, "find: argv[1] has to be a DIR, but it is a file...\n");
            break;

        case T_DIR:
            if (strlen(path) + 1 + strlen(filename) + 1 > sizeof(buf)) {
                printf("find: path too long\n");
                break;
            }          
            strcpy(buf, path);
            p = buf + strlen(path);
            *p++ = '/';

            while(read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if (stat(buf, &st) < 0) {
                    printf("find: cannot stat %s\n", buf);
                    continue;
                }
                switch(st.type) {
                    case T_FILE:
                        if (!strcmp(de.name, filename)) {
                            printf("%s/%s\n", path, de.name);
                        }
                        break;
                    
                    case T_DIR:
                        if ((!strcmp(de.name, ".")) || (!strcmp(de.name, ".."))) {
                            continue;
                        }
                        find(buf, filename);
                        break;
                }
            }
    }
    close(fd);
}

int
main(int argc, char *argv[])
{
    int i;

    if (argc < 3) {
        fprintf(2, "usage: find files in directory...\n");
        exit(1);
    }

    for (i = 2; i < argc; ++i) {
        find(argv[1], argv[i]);
    }

    exit(0);
}