/*	$Id: getdents.c,v 1.7 2005/08/20 05:25:16 yamt Exp $	*/

/*-
 * Copyright (c)2004 YAMAMOTO Takashi,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <err.h>
#include <dirent.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_dents(FILE *, const void *, int);
int main(int, char *[]);

void
print_dents(FILE *fp, const void *vp, int sz)
{
	const char *cp = vp;
	const char *ep = cp + sz;
	const struct dirent *d;

	while (cp < ep) {
		d = (const void *)cp;
		fprintf(fp, "fileno=%" PRIu64
		    ", type=%d, reclen=%d, len=%d, %s\n",
		    (uint64_t)d->d_fileno, (int)d->d_type, (int)d->d_reclen,
		    (int)d->d_namlen, d->d_name);
		cp += d->d_reclen;
	}
#if 1
	{
		int i;

		for (cp = vp, i = 0; cp < ep; cp++, i++) {
			if ((i % 16) == 0)
				fprintf(fp, "%08tx:", cp - (const char *)vp);
			fprintf(fp, "%02x ", (int)(unsigned char)*cp);
			if ((i % 16) == 15)
				fprintf(fp, "\n");
		}
	}
#endif
}

struct ent {
	off_t off;
	int sz;
	char buf[DIRBLKSIZ];
};

int
main(int argc, char *argv[])
{
	int fd;
	off_t off;
	int ret;
	const char *path;
	int nblks = 0;
	struct ent *blks = NULL;
	int i;
	int count;

	if (argc < 2)
		errx(EXIT_FAILURE, "arg");
	path = argv[1];
	if (argc > 2)
		off = strtoull(argv[2], 0, 0);
	else
		off = 0;

	printf("dir=%s\n", path);
	fd = open(path, O_RDONLY);
	if (fd == -1)
		err(EXIT_FAILURE, "open");
	printf("seek: off=%" PRIx64 "(%" PRIu64 ")\n",
	    (uint64_t)off, (uint64_t)off);
	if (off != lseek(fd, off, SEEK_SET))
		err(EXIT_FAILURE, "lseek");

	printf("start reading..\n");
	do {
		struct ent *p;
		blks = realloc(blks, sizeof(*blks) * (nblks + 1));
		if (blks == NULL)
			err(EXIT_FAILURE, "realloc");
		p = &blks[nblks];
		memset(p, 0, sizeof(*p));
		p->off = off = lseek(fd, (off_t)0, SEEK_CUR);
		if (off == (off_t)-1)
			err(EXIT_FAILURE, "lseek");
		printf("off=%" PRIx64 "(%" PRIu64 ")\n",
		    (uint64_t)off, (uint64_t)off);
		p->sz = ret = getdents(fd, p->buf, DIRBLKSIZ);
		printf("getdents: %d\n", ret);
		if (ret == -1)
			err(EXIT_FAILURE, "getdents");
		nblks++;
		print_dents(stdout, p->buf, ret);
	} while (ret > 0);
	printf("%d blks read\n", nblks);

#if 1
	printf("re-open the file\n");
	if (close(fd))
		err(EXIT_FAILURE, "close");
	fd = open(path, O_RDONLY);
	if (fd == -1)
		err(EXIT_FAILURE, "open");
#endif

	printf("starting random read...\n");
	for (count = nblks * 4 + 10; count; count--) {
		char buf[DIRBLKSIZ];
		struct ent *p;
		int differ = 0;

		i = rand() % nblks;
		p = &blks[i];
		off = lseek(fd, p->off, SEEK_SET);
		if (off == (off_t)-1)
			err(1, "seek");
		printf("off=%" PRIx64 "(%" PRIu64 ")\n",
		    (uint64_t)off, (uint64_t)off);
		ret = getdents(fd, buf, DIRBLKSIZ);
		printf("getdents: %d\n", ret);
		if (ret == -1)
			err(EXIT_FAILURE, "getdents");
		if (p->sz != ret) {
			fflush(NULL);
			fprintf(stderr, "off=%" PRIx64
			    ": different sz %d != %d\n",
			    (uint64_t)off, p->sz, ret);
			differ = 1;
		} else if (memcmp(p->buf, buf, (size_t)ret)) {
			fflush(NULL);
			fprintf(stderr, "off=%" PRIx64 ": different data\n",
			    (uint64_t)off);
			fprintf(stderr, "previous:\n");
			print_dents(stderr, p->buf, p->sz);
			fprintf(stderr, "now:\n");
			print_dents(stderr, buf, ret);
			differ = 1;
		}
		if (differ) {
			const struct dirent *d1 = (void *)p->buf;
			const struct dirent *d2 = (void *)buf;

			if (p->sz == 0 || ret == 0 ||
			    d1->d_fileno != d2->d_fileno ||
#if defined(DT_UNKNOWN)
			    (d1->d_type != DT_UNKNOWN &&
			     d2->d_type != DT_UNKNOWN &&
			     d1->d_type != d2->d_type) ||
#endif /* defined(DT_UNKNOWN) */
			    d1->d_namlen != d2->d_namlen ||
			    memcmp(d1->d_name, d2->d_name, d1->d_namlen)) {
				fprintf(stderr, "fatal difference\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	exit(EXIT_SUCCESS);
	/* NOTREACHED */
}
