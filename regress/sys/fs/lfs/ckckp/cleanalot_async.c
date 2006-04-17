#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void dirname(int n, char *s)
{
	if (n == 0) {
		strcat(s, "/0");
		mkdir(s);
	}
        while (n) {
                sprintf(s + strlen(s), "/%x", n & 0xf);
                n >>= 4;
		mkdir(s);
        }
}

/*
 * Write a file, creating the directory if necessary.
 */
int write_file(int gen, int n, int plex, char *buf, int size)
{
        FILE *fp;
        char s[1024], *t;
        int r;

        sprintf(s, "dir_%x_%x", plex, gen);
        dirname(n, s);
	strcat(s, ".file");

        // printf("write file %d.%d.%x: %s\n", gen, plex, n, s);

        fp = fopen(s, "wb");
        if (fp == NULL)
                return 0;
        r = fwrite(buf, size, 1, fp);
        fclose(fp);

        return r;
}

int write_dirs(int gen, int size, int plex)
{
        int i, j, tot;
        char s[1024];
        char *buf;

        /* Create all base dirs */
        for (i = 0; i < plex; i++) {
                sprintf(s, "dir_%x_%x", i, gen);
                if (mkdir(s, 0700) != 0)
                        return 0;
        }

        /* Write files */
        buf = malloc(size);
        if (buf == NULL)
                return 0;
        tot = 0;
        for (i = 0; ; i++) {
                for (j = 0; j < plex; j++) {
                        if (write_file(gen, i, j, buf, size) == 0) {
                                free(buf);
                                return tot;
                        }
                        ++tot;
                }
        }
        /* NOTREACHED */
}

int main(int argc, char **argv)
{
        int c, i, j;
        int bs = 0;
        int count = 0;
        int plex = 2;
        char cmd[1024];

        while((c = getopt(argc, argv, "b:n:p:")) != -1) {
                switch(c) {
                    case 'b':
                            bs = atoi(optarg);
                            break;
                    case 'n':
                            count = atoi(optarg);
                            break;
                    case 'p':
                            plex = atoi(optarg);
                            break;
		    default:
                            exit(1);
                }
        }

        /*
         * Process old-style, non-flag parameters
         */
        if (count == 0) {
                if (argv[optind] != NULL)
                        count = atoi(argv[optind]);
        }
        if (bs == 0 && getenv("BS") != NULL)
                bs = atoi(getenv("BS"));
        if (bs == 0)
                bs = 16384;
        if (plex == 0)
                plex = 2;

        for (i = 0; count == 0 || i < count; i++) {
                if (count)
                        printf("::: begin iteration %d/%d\n", i, count);
                else
                        printf("::: begin iteration %d\n", i);

                for (j = 0; ; j++) {
                        if ((c = write_dirs(j, bs, plex)) == 0)
                                break;
                	printf("%d: %d files of size %d\n", j, c, bs);
                        sprintf(cmd, "rm -rf dir_%x_%x", plex - 1, j);
                        system(cmd);
			sync();
                }
                printf("%d files of size %d\n", j * plex, bs);
		system("df -k .");
        }
}
