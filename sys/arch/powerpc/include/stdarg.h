/*	$NetBSD: stdarg.h,v 1.3 1998/12/02 14:23:03 tsubai Exp $	*/

#ifndef	_PPC_STDARG_H_
#define	_PPC_STDARG_H_

#include <machine/ansi.h>

#ifndef	_STDARG_H
#define	_STDARG_H
#endif
#include <powerpc/va-ppc.h>

typedef	__gnuc_va_list	va_list;

#endif /* ! _PPC_STDARG_H_ */
