/*
 * Written by J.T. Conklin, December 12, 1994
 * Public domain.
 */

#include <sys/cdefs.h>

__weak_reference(_sys_errlist, sys_errlist);
__weak_reference(_sys_errlist, __sys_errlist); /* Backwards compat with v.12 */
