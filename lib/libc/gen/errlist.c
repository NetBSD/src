/*
 * Written by J.T. Conklin, 10/06/94
 * Public domain.
 */

#include <sys/cdefs.h>

#ifdef __weak_reference
__weak_reference(__sys_errlist, sys_errlist);
__weak_reference(__sys_nerr, sys_nerr);
#else

/*
 * Without weak references, we're forced to have to have two copies of
 * the error message string table in the library.  Fortunately, unless
 * a program uses both sys_errlist[] and strerror(), only one of the
 * copies will be linked into the executable.
 */

#define __sys_errlist	sys_errlist
#define __sys_nerr	sys_nerr
#include "_errlst.c"
#endif
