/*	$NetBSD: stdarg.h,v 1.2 1997/04/16 23:02:34 thorpej Exp $	*/

#ifndef	_PPC_STDARG_H_
#define	_PPC_STDARG_H_

#include <machine/ansi.h>

#ifndef	_STDARG_H
#define	_STDARG_H
#endif
#include <machine/va-ppc.h>

typedef	__gnuc_va_list	va_list;

#endif /* ! _PPC_STDARG_H_ */
