/*
 * Written by J.T. Conklin, December 12, 1994
 * Public domain.
 */

#include <sys/cdefs.h>

__weak_reference(_sys_siglist, sys_siglist);
__weak_reference(_sys_siglist, __sys_siglist); /* Backwards compat with v.12 */
