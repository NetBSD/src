/*
 *	$Id: salibc.h,v 1.1 1993/10/16 07:32:22 cgd Exp $
 */

#include <sys/types.h>

/* string functions */

void bcopy __P((const void *, void *, size_t));
void *memset __P((void *, int, size_t));
char *strerror __P((int));

/* misc routines/definitions */

extern int errno;
int printf __P((const char *, ...));
