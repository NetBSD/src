#	$NetBSD: Makefile,v 1.7 1998/01/09 08:05:47 perry Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

# -DEXTENDED 
# 	if you want the paste & spaste macros.

PROG=	m4
CPPFLAGS+=-DEXTENDED
SRCS=	eval.c expr.c look.c main.c misc.c
MAN=	m4.1

.include <bsd.prog.mk>
