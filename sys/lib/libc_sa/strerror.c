/*	$NetBSD: strerror.c,v 1.3 1995/04/22 14:01:50 cgd Exp $	*/

#include <sys/types.h>
#include <sys/errno.h>

size_t	strlen __P((const char *));				/* XXX */

char *
strerror(err)
	int err;
{
	static char ebuf[1024] = "Unknown error: code ";
	char *p;
	int length;

	switch (err) {
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
			*p++ = "0123456789"[err % 10];
		} while (err /= 10);
		*p = '\0';
		return ebuf;
	}
}
