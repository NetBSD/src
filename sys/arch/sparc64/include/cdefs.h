/*	$NetBSD: cdefs.h,v 1.1.1.1 1998/06/20 04:58:51 eeh Exp $	*/

/*
 * Written by J.T. Conklin <jtc@wimsey.com> 01/17/95.
 * Public domain.
 */

#ifndef	_MACHINE_CDEFS_H_
#define	_MACHINE_CDEFS_H_

#ifdef __ELF__

#define	_C_LABEL(x)	x

#ifdef __GNUC__
#define	__RENAME(x)	__asm__(___STRING(_C_LABEL(x)))
#else
#define __RENAME(x)
#endif

#define	__DO_NOT_DO_WEAK__		/* NO WEAK SYMS IN LIBC YET */

#ifndef __DO_NOT_DO_WEAK__
#define	__indr_reference(sym,alias)	/* nada, since we do weak refs */
#endif /* !__DO_NOT_DO_WEAK__ */

#ifdef __STDC__

#ifndef __DO_NOT_DO_WEAK__
#define	__weak_alias(alias,sym)						\
    __asm__(".weak " #alias " ; " #alias " = " #sym);
#endif /* !__DO_NOT_DO_WEAK__ */
#define	__warn_references(sym,msg)					\
    __asm__(".section .gnu.warning." #sym " ; .ascii \"" msg "\" ; .text");

#else /* !__STDC__ */

#ifndef __DO_NOT_DO_WEAK__
#define	__weak_alias(alias,sym)						\
    __asm__(".weak alias ; alias = sym");
#endif /* !__DO_NOT_DO_WEAK__ */
#define	__warn_references(sym,msg)					\
    __asm__(".section .gnu.warning.sym ; .ascii msg ; .text");

#endif /* !__STDC__ */

#else /* !__ELF__ */

#define	_C_LABEL(x)	__CONCAT(_,x)

#ifdef __GNUC__
#define	__RENAME(x)	__asm__(___STRING(_C_LABEL(x)))
#else
#define __RENAME(x)
#endif

#ifdef __GNUC__
#ifdef __STDC__
#define __indr_reference(sym,alias)	\
	__asm__(".stabs \"_" #alias "\",11,0,0,0");	\
	__asm__(".stabs \"_" #sym "\",1,0,0,0");
#define __warn_references(sym,msg)	\
	__asm__(".stabs \"" msg "\",30,0,0,0");		\
	__asm__(".stabs \"_" #sym "\",1,0,0,0");
#else
#define __indr_reference(sym,alias)	\
	__asm__(".stabs \"_/**/alias\",11,0,0,0");	\
	__asm__(".stabs \"_/**/sym\",1,0,0,0");
#define __warn_references(sym,msg)	\
	__asm__(".stabs msg,30,0,0,0");			\
	__asm__(".stabs \"_/**/sym\",1,0,0,0");
#endif
#else
#define __indr_reference(sym,alias)
#define __warn_references(sym,msg)
#endif

#endif /* __ELF__ */

#endif /* !_MACHINE_CDEFS_H_ */



