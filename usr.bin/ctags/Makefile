#	$NetBSD: Makefile,v 1.5 2002/01/31 19:26:34 tv Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	ctags
CPPFLAGS+=-I${.CURDIR}
SRCS=	C.c ctags.c fortran.c lisp.c print.c tree.c yacc.c

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
