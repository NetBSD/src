/* liballoc - flex run-time memory allocation */

/* $Header: /cvsroot/src/usr.bin/lex/lib/Attic/liballoc.c,v 1.1 1993/12/02 19:14:28 jtc Exp $ */

#include <stdio.h>

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

/* The following is only used by bison/alloca. */
void *yy_flex_xmalloc( size )
int size;
	{
	void *result = yy_flex_alloc( size );

	if ( ! result  )
		{
		fprintf( stderr,
			"flex memory allocation failed in yy_flex_xmalloc()" );
		exit( 1 );
		}

	return result;
	}
