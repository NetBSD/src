#	$NetBSD: Makefile,v 1.27 2023/06/03 09:09:13 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/19/93

USE_FORT?= yes	# network client/server
PROG=	routed
SRCS=	if.c input.c main.c output.c parms.c radix.c rdisc.c table.c trace.c
MAN=	routed.8
SUBDIR=	rtquery

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

# By popular demand...
MLINKS=	routed.8 rdisc.8
#COPTS+=-g -DDEBUG -Wall

COPTS.main.c = -Wno-format-nonliteral
COPTS.trace.c = -Wno-format-nonliteral

COPTS.if.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
