#	$NetBSD: Makefile,v 1.11 2001/11/14 14:22:09 tv Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

# -DEXTENDED 
# 	if you want the paste & spaste macros.

PROG=		m4
CPPFLAGS+=	-DEXTENDED -Dunix
SRCS=		eval.c expr.c gnum4.c look.c main.c misc.c trace.c

WARNS=1

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
