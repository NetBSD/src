/*	$NetBSD: __res_close.c,v 1.1 1997/04/22 06:55:38 mrg Exp $	*/

/*
 * written by matthew green, 22/04/97.
 * public domain.
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(__res_close,_res_close);
#else

#include <sys/types.h>
#include <resolv.h>

extern void __res_close __P((void));

#undef _res_close
void
_res_close()
{

	__res_close();
}

#endif
