/*	$NetBSD: main.c,v 1.2 1998/01/05 07:03:53 perry Exp $	*/

#include <unistd.h>
#include "gzip.h"

#ifdef BOOT
static char src[1024*1024];
static char dst[2048*1024];

void
debug_print(m)
    const char *m;
{
    write(2, m, strlen(m));
    write(2, "\n", 1);
}
#endif

/* for debug */
int
main()
{
#ifdef BOOT
    char *p;
    int len;

    for (p = src; (len = read(0, p, src + sizeof src - p)) > 0; p += len)
	;
    if (unzip(src, dst))
#else
    if (unzip(0,0))
#endif
	return 1;
#ifdef BOOT
    write (1, dst, udst_cnt);
#endif
    return 0;
}
