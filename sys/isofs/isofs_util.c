/*
 *	$Id: isofs_util.c,v 1.4 1993/09/03 04:37:56 cgd Exp $
 */

#include "param.h"
#include "systm.h"
#include "namei.h"
#include "resourcevar.h"
#include "kernel.h"
#include "file.h"
#include "stat.h"
#include "buf.h"
#include "proc.h"
#include "conf.h"
#include "mount.h"
#include "vnode.h"
#include "specdev.h"
#include "fifo.h"
#include "malloc.h"
#include "dir.h"

#include "iso.h"
#include "dirent.h"
#include "machine/endian.h"

int
isonum_711 (p)
unsigned char *p;
{
	return (*p);
}

int
isonum_712 (p)
signed char *p;
{
	return (*p);
}

int
isonum_721 (p)
unsigned char *p;
{
	/* little endian short */
#if BYTE_ORDER != LITTLE_ENDIAN
	printf ("isonum_721 called on non little-endian machine!\n");
#endif

	return *(short *)p;
}

int
isonum_722 (p)
unsigned char *p;
{
        /* big endian short */
#if BYTE_ORDER != BIG_ENDIAN
        printf ("isonum_722 called on non big-endian machine!\n");
#endif

	return *(short *)p;
}

int
isonum_723 (p)
unsigned char *p;
{
#if BYTE_ORDER == BIG_ENDIAN
        return isonum_722 (p + 2);
#elif BYTE_ORDER == LITTLE_ENDIAN
	return isonum_721 (p);
#else
	printf ("isonum_723 unsupported byte order!\n");
	return 0;
#endif
}

int
isonum_731 (p)
unsigned char *p;
{
        /* little endian long */
#if BYTE_ORDER != LITTLE_ENDIAN
        printf ("isonum_731 called on non little-endian machine!\n");
#endif

	return *(long *)p;
}

int
isonum_732 (p)
unsigned char *p;
{
        /* big endian long */
#if BYTE_ORDER != BIG_ENDIAN
        printf ("isonum_732 called on non big-endian machine!\n");
#endif

	return *(long *)p;
}

int
isonum_733 (p)
unsigned char *p;
{
#if BYTE_ORDER == BIG_ENDIAN
        return isonum_732 (p + 4);
#elif BYTE_ORDER == LITTLE_ENDIAN
	return isonum_731 (p);
#else
	printf ("isonum_733 unsupported byte order!\n");
	return 0;
#endif
}

/*
 * translate and compare a filename
 */
isofncmp(char *fn, int fnlen, char *isofn, int isolen) {
	int fnidx;

	fnidx = 0;
	for (fnidx = 0; fnidx < isolen; fnidx++, fn++) {
		char c = *isofn++;

		if (fnidx > fnlen)
			return (0);

		if (c >= 'A' && c <= 'Z') {
			if (c + ('a' - 'A') !=  *fn)
				return(0);
			else
				continue;
		}
		if (c == ';')
			return ((fnidx == fnlen));
		if (c != *fn)
			return (0);
	}
	return (1);
}

/*
 * translate a filename
 */
void
isofntrans(char *infn, int infnlen, char *outfn, short *outfnlen) {
	int fnidx;

	fnidx = 0;
	for (fnidx = 0; fnidx < infnlen; fnidx++) {
		char c = *infn++;

		if (c >= 'A' && c <= 'Z')
			*outfn++ = c + ('a' - 'A');
		else if (c == ';') {
			*outfnlen = fnidx;
			return;
		} else
			*outfn++ = c;
	}
	*outfnlen = infnlen;
}
