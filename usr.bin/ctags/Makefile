#	$NetBSD: Makefile,v 1.9.22.1 2008/09/18 04:29:10 wrstuden Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	ctags
CPPFLAGS+=-I${.CURDIR}
SRCS=	C.c ctags.c fortran.c lisp.c print.c tree.c yacc.c

WARNS=	3

.include <bsd.prog.mk>

.if !defined(HOSTPROGNAME)
.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.ctags.c+=	-Wno-pointer-sign
.endif
.endif
