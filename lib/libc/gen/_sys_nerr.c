/*
 * Written by J.T. Conklin, December 12, 1994
 * Public domain.
 */

#include <sys/cdefs.h>

#ifdef __weak_reference
__weak_reference(_sys_nerr, sys_nerr);
__weak_reference(_sys_nerr, __sys_nerr); /* Backwards compat with v.12 */
#endif
