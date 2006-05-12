#	$NetBSD: Makefile,v 1.9 2006/05/12 01:18:25 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	ctags
CPPFLAGS+=-I${.CURDIR}
SRCS=	C.c ctags.c fortran.c lisp.c print.c tree.c yacc.c

WARNS=	3

.include <bsd.prog.mk>

.if !defined(HOSTPROGNAME)
.if ${HAVE_GCC} == 4
COPTS.ctags.c+=	-Wno-pointer-sign
.endif
.endif
