/*	$NetBSD: cdefs_aout.h,v 1.10 2002/05/11 11:57:14 itohy Exp $	*/

/*
 * Written by J.T. Conklin <jtc@wimsey.com> 01/17/95.
 * Public domain.
 */

#ifndef _SYS_CDEFS_AOUT_H_
#define	_SYS_CDEFS_AOUT_H_

#define	_C_LABEL(x)	__CONCAT(_,x)

#if __STDC__
#define	___RENAME(x)	__asm__(___STRING(_C_LABEL(x)))
#else
#define	___RENAME(x)	____RENAME(_/**/x)
#define	____RENAME(x)	__asm__(___STRING(x))
#endif

#ifdef __GNUC__
#if __STDC__
#define	__indr_reference(sym,alias)					\
	__asm__(".stabs \"_" #alias "\",11,0,0,0");			\
	__asm__(".stabs \"_" #sym "\",1,0,0,0");

#define	__warn_references(sym,msg)					\
	__asm__(".stabs \"" msg "\",30,0,0,0");				\
	__asm__(".stabs \"_" #sym "\",1,0,0,0");
#else /* __STDC__ */
#define	__indr_reference(sym,alias)					\
	__asm__(".stabs \"_/**/alias\",11,0,0,0");			\
	__asm__(".stabs \"_/**/sym\",1,0,0,0");

#define	__warn_references(sym,msg)					\
	__asm__(".stabs msg,30,0,0,0");					\
	__asm__(".stabs \"_/**/sym\",1,0,0,0");
#endif /* __STDC__ */
#else /* __GNUC__ */
#define	__warn_references(sym,msg)
#endif /* __GNUC__ */

#if defined(__sh__)		/* XXX SH COFF */
#undef __indr_reference(sym,alias)
#undef __warn_references(sym,msg)
#define __warn_references(sym,msg)
#endif

#define __IDSTRING(_n,_s)						\
	__asm__(".data ; .asciz \"" _s "\" ; .text")

#undef __KERNEL_RCSID

#define __RCSID(_s)	__IDSTRING(rcsid,_s)
#define __SCCSID(_s)
#define __SCCSID2(_s)
#if 0	/* XXX userland __COPYRIGHTs have \ns in them */
#define __COPYRIGHT(_s)	__IDSTRING(copyright,_s)
#else
#define __COPYRIGHT(_s)							\
	static const char copyright[] __attribute__((__unused__)) = _s
#endif

#if defined(USE_KERNEL_RCSIDS) || !defined(_KERNEL)
#define	__KERNEL_RCSID(_n,_s) __IDSTRING(__CONCAT(rcsid,_n),_s)
#else
#define	__KERNEL_RCSID(_n,_s)
#endif
#define	__KERNEL_SCCSID(_n,_s)
#define	__KERNEL_COPYRIGHT(_n, _s) __IDSTRING(__CONCAT(copyright,_n),_s)

#endif /* !_SYS_CDEFS_AOUT_H_ */
