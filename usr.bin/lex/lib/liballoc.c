/* liballoc - flex run-time memory allocation */

/* $Header: /cvsroot/src/usr.bin/lex/lib/Attic/liballoc.c,v 1.2 1993/12/06 19:26:02 jtc Exp $ */

#ifdef STDC_HEADERS

#include <stdlib.h>

#else /* not STDC_HEADERS */

void *malloc();
void *realloc();
void free();

#endif /* not STDC_HEADERS */


void *yy_flex_alloc( size )
int size;
	{
	return (void *) malloc( (unsigned) size );
	}

void *yy_flex_realloc( ptr, size )
void *ptr;
int size;
	{
	return (void *) realloc( ptr, (unsigned) size );
	}

void yy_flex_free( ptr )
void *ptr;
	{
	free( ptr );
	}
