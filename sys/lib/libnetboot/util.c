#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include "../include/stdarg.h"

void
#ifdef __STDC__
panic(const char *fmt, ...)
#else
panic(fmt /*, va_alist */)
	char *fmt;
#endif
{
    va_list ap;

    va_start(ap, fmt);
    printf(fmt, ap);
    printf("\n");
    va_end(ap);
    machdep_stop();
}


/* Similar to inet_ntoa() */
char *
intoa(addr)
	n_long addr;
{
	register char *cp;
	register u_int byte;
	register int n;
	static char buf[sizeof(".xxx.xxx.xxx.xxx")];

	cp = &buf[sizeof buf];
	*--cp = '\0';

	n = 4;
	do {
		byte = addr & 0xff;
		*--cp = byte % 10 + '0';
		byte /= 10;
		if (byte > 0) {
			*--cp = byte % 10 + '0';
			byte /= 10;
			if (byte > 0)
				*--cp = byte + '0';
		}
		*--cp = '.';
		addr >>= 8;
	} while (--n > 0);

	return (cp + 1);
}

char *strerror(errnum)
     int errnum;
{
    char ebuf[1024] = "Unknown error: code ";
    char *p;
    int length;



    switch (errnum) {
    case EPERM:
	return "Permission Denied";
    case ENOENT:
	return "No such file or directory";
    case ESTALE:
	return "Stale NFS file handle";
    default:
	length = strlen(ebuf);
	p = ebuf+length;
	do {
	    *p++ = "0123456789"[errnum %10];
	} while (errnum /= 10);
	*p = '\0';
	return ebuf;
    }
}


/*
 * Convert Ethernet address to printable (loggable) representation.
 */
static char digits[] = "0123456789abcdef";
char *
ether_sprintf(ap)
        register u_char *ap;
{
	register i;
	static char etherbuf[18];
	register char *cp = etherbuf;

	for (i = 0; i < 6; i++) {
		*cp++ = digits[*ap >> 4];
		*cp++ = digits[*ap++ & 0xf];
		*cp++ = ':';
	}
	*--cp = 0;
	return (etherbuf);
}

void bzero(addr, len)
     char *addr;
     int len;
{
    while (len--)
	*addr++ = '\0';
}
