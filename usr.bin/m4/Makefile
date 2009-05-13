#	$NetBSD: Makefile,v 1.13.14.1 2009/05/13 19:19:55 jym Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

# -DEXTENDED 
# 	if you want the paste & spaste macros.

WARNS?=	1		# XXX: -Wcast-qual -Wshadow issues

PROG=		m4
CPPFLAGS+=	-DEXTENDED -Dunix
SRCS=		eval.c expr.c gnum4.c look.c main.c misc.c trace.c

.if (${HOSTPROG:U} == "")
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
.endif

.include <bsd.prog.mk>
