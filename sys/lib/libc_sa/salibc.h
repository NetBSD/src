/*	$NetBSD: salibc.h,v 1.2 1994/10/26 05:36:54 cgd Exp $	*/

#include <sys/types.h>

/* string functions */

void bcopy __P((const void *, void *, size_t));
void *memset __P((void *, int, size_t));
char *strerror __P((int));

/* misc routines/definitions */

extern int errno;
int printf __P((const char *, ...));
