/*	$NetBSD: cdefs.h,v 1.5 1999/01/11 11:02:16 christos Exp $	*/

/*
 * Written by J.T. Conklin <jtc@wimsey.com> 01/17/95.
 * Public domain.
 */

#ifndef	_MACHINE_CDEFS_H_
#define	_MACHINE_CDEFS_H_

#ifdef __ELF__
# define _C_LABEL(x)	x
#else
# define _C_LABEL(x)	__CONCAT(_,x)
#endif

#ifdef __GNUC__
# define __RENAME(x)	__asm__(___STRING(_C_LABEL(x)))
#endif

#ifdef __ELF__

# define __DO_NOT_DO_WEAK__		/* NO WEAK SYMS IN LIBC YET */

# ifndef __DO_NOT_DO_WEAK__
#  define __indr_reference(sym,alias)	/* nada, since we do weak refs */
# endif /* !__DO_NOT_DO_WEAK__ */

# ifdef __STDC__

#  ifndef __DO_NOT_DO_WEAK__
#   define __weak_alias(alias,sym)					\
    __asm__(".weak " #alias " ; " #alias " = " #sym);
#  endif /* !__DO_NOT_DO_WEAK__ */
#  define	__warn_references(sym,msg)				\
    __asm__(".section .gnu.warning." #sym " ; .ascii \"" msg "\" ; .text");

# else /* !__STDC__ */

#  ifndef __DO_NOT_DO_WEAK__
#   define	__weak_alias(alias,sym)					\
    __asm__(".weak alias ; alias = sym");
#  endif /* !__DO_NOT_DO_WEAK__ */
#  define	__warn_references(sym,msg)				\
    __asm__(".section .gnu.warning.sym ; .ascii msg ; .text");
# endif /* __STDC__ */
#else /* !__ELF__ */
# ifdef __GNUC__
#  ifdef __STDC__
#   define __indr_reference(sym,alias)	\
	__asm__(".stabs \"_" #alias "\",11,0,0,0");	\
	__asm__(".stabs \"_" #sym "\",1,0,0,0");
#   define __warn_references(sym,msg)	\
	__asm__(".stabs \"" msg "\",30,0,0,0");		\
	__asm__(".stabs \"_" #sym "\",1,0,0,0");
#  else /* __STDC__ */
#   define __indr_reference(sym,alias)	\
	__asm__(".stabs \"_/**/alias\",11,0,0,0");	\
	__asm__(".stabs \"_/**/sym\",1,0,0,0");
#   define __warn_references(sym,msg)	\
	__asm__(".stabs msg,30,0,0,0");			\
	__asm__(".stabs \"_/**/sym\",1,0,0,0");
#  endif /* __STDC__ */
# else /* __GNUC__ */
#  define	__warn_references(sym,msg)
# endif /* __GNUC__ */
#endif /* __ELF__ */

#endif /* !_MACHINE_CDEFS_H_ */
