#	$NetBSD: Makefile,v 1.9 2000/10/11 14:46:11 is Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

# -DEXTENDED 
# 	if you want the paste & spaste macros.

PROG=	m4
CPPFLAGS+=-DEXTENDED -Dunix
SRCS=	eval.c expr.c look.c main.c misc.c
MAN=	m4.1
.include <bsd.prog.mk>
