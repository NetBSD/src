/*	$NetBSD: varargs.h,v 1.3 1998/12/02 14:23:03 tsubai Exp $	*/

#ifndef	_PPC_VARARGS_H_
#define	_PPC_VARARGS_H_

#include <machine/ansi.h>

#ifndef	_VARARGS_H
#define	_VARARGS_H
#endif
#include <powerpc/va-ppc.h>

typedef	__gnuc_va_list	va_list;

#undef	_BSD_VA_LIST

#endif /* ! _PPC_VARARGS_H_ */
