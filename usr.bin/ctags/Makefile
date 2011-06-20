#	$NetBSD: Makefile,v 1.12 2011/06/20 07:44:01 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	ctags
CPPFLAGS+=-I${.CURDIR}
SRCS=	C.c ctags.c fortran.c lisp.c print.c tree.c yacc.c

.include <bsd.prog.mk>

.if !defined(HOSTPROGNAME)
.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.ctags.c+=	-Wno-pointer-sign
.endif
.endif
