/* ===-- clzti2.c - Implement __clzti2 -------------------------------------===
 *
 *      	       The LLVM Compiler Infrastructure
 *
 * This file is dual licensed under the MIT and the University of Illinois Open
 * Source Licenses. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 *
 * This file implements __clzti2 for the compiler_rt library.
 *
 * ===----------------------------------------------------------------------===
 */

#include "int_lib.h"

#if __x86_64

/* Returns: the number of leading 0-bits */

/* Precondition: a != 0 */

si_int
__clzti2(ti_int a)
{
    twords x;
    x.all = a;
    const di_int f = -(x.s.high == 0);
    return __builtin_clzll((x.s.high & ~f) | (x.s.low & f)) +
           ((si_int)f & ((si_int)(sizeof(di_int) * CHAR_BIT)));
}

#endif /* __x86_64 */
