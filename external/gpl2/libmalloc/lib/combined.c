/*
 * this file (combined.c) is malloc.c, free.c, and realloc.c, combined into
 * one file, because the malloc.o in libc defined malloc, realloc, and free,
 * and libc sometimes invokes realloc, which can greatly confuse things
 * in the linking process...
 *
 *	$Id: combined.c,v 1.2 2021/03/03 21:46:43 christos Exp $
 */

#include "malloc.c"
#include "free.c"
#include "realloc.c"

/* jemalloc defines these */
void _malloc_prefork(void);
void _malloc_postfork(void); 
void _malloc_postfork_child(void);
void _malloc_prefork(void) {}
void _malloc_postfork(void) {}
void _malloc_postfork_child(void) {}
