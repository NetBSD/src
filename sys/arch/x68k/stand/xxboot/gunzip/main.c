/*	$NetBSD: main.c,v 1.1 1998/09/01 20:03:47 itohy Exp $	*/

#include <unistd.h>
#include "gzip.h"

#ifdef BOOT
static char src[1024*1024];
static char dst[2048*1024];

#ifdef __GNUC__
volatile
#endif
void
BOOT_ERROR(m)
    const char *m;
{
    write(2, m, strlen(m));
    write(2, "\n", 1);
    _exit(1);
    for(;;);	/* make gcc happy */
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
    unzip(src, dst);
#else
    unzip(0,0);
#endif
#ifdef BOOT
    write (1, dst, udst_cnt);
#endif
    return 0;
}
