/*	$NetBSD: cdefs.h,v 1.1.2.1 1997/10/22 06:16:24 thorpej Exp $	*/

#ifndef	_MACHINE_CDEFS_H_
#define	_MACHINE_CDEFS_H_

#define _C_LABEL(x)	_STRING(x)

#ifdef __GNUC__
#define	__RENAME(x)	__asm__(x)
#endif

#define	__DO_NOT_DO_WEAK__		/* NO WEAK SYMS IN LIBC YET */

#ifndef __DO_NOT_DO_WEAK__
#define	__indr_reference(sym, alias)	/* use weak symbols instead */
#endif

#ifdef	__STDC__

#ifndef	__DO_NOT_DO_WEAK__
#define	 __weak_alias(alias, sym)			\
	__asm__(".weak " #alias " ; " #alias " = " #sym)
#endif

#define	__warn_references(sym, msg)			\
	__asm__(".section .gnu.warning." #sym " ; .ascii \"" msg "\" ; .text")

#else /* ! __STDC__ */

#ifndef	__DO_NOT_DO_WEAK__
#define	__weak_alias(alias, sym)			\
	__asm__(".weak alias ; alias = sym")
#endif

#define	__warn_references(sym, msg)			\
	__asm__(".section .gnu.warning.sym ; .ascii msg ; .text")

#endif /* __STDC__ */

#endif /* !_MACHINE_CDEFS_H_ */
