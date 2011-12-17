#	$NetBSD: Makefile,v 1.1 2011/12/17 13:24:18 tsutsui Exp $

USE_FORT?= yes	# network server

PROG=	isibootd
SRCS=	isibootd.c
MAN=	isibootd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.own.mk>
.include <bsd.prog.mk>
