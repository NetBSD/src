/*
 *	$Id: strerror.c,v 1.1 1993/10/13 11:16:25 cgd Exp $
 */

#include <sys/errno.h>

char *
strerror(err)
	int err;
{
	char ebuf[1024] = "Unknown error: code ";
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
