/*
 * Written by J.T. Conklin, 10/06/94
 * Public domain.
 */

#include <sys/cdefs.h>

#ifdef __weak_reference
__weak_reference(__sys_siglist, sys_siglist);
#else

/*
 * Without weak references, we're forced to have to have two copies of
 * the signal name string table in the library.  Fortunately, unless
 * a program uses both sys_siglist[] and strsignal(), only one of the
 * copies will be linked into the executable.
 */

#define __sys_siglist	sys_siglist
#include "_siglist.c"
#endif
