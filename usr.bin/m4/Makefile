#	$NetBSD: Makefile,v 1.10 2001/11/14 06:16:07 tv Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

# -DEXTENDED 
# 	if you want the paste & spaste macros.

PROG=		m4
CPPFLAGS+=	-DEXTENDED -Dunix
SRCS=		eval.c expr.c gnum4.c look.c main.c misc.c trace.c

WARNS=1

.include <bsd.prog.mk>
