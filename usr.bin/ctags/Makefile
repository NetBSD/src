#	$NetBSD: Makefile,v 1.10.6.1 2009/05/13 19:19:47 jym Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	ctags
CPPFLAGS+=-I${.CURDIR}
SRCS=	C.c ctags.c fortran.c lisp.c print.c tree.c yacc.c

.include <bsd.prog.mk>

.if !defined(HOSTPROGNAME)
.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.ctags.c+=	-Wno-pointer-sign
.endif
.endif
