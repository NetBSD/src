#	$NetBSD: Makefile,v 1.13 2007/10/05 07:36:45 lukem Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

# -DEXTENDED 
# 	if you want the paste & spaste macros.

PROG=		m4
CPPFLAGS+=	-DEXTENDED -Dunix
SRCS=		eval.c expr.c gnum4.c look.c main.c misc.c trace.c

.if (${HOSTPROG:U} == "")
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
.endif

WARNS=1

.include <bsd.prog.mk>
