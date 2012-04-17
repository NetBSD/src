#	$NetBSD: Makefile,v 1.1.4.2 2012/04/17 00:09:48 yamt Exp $

USE_FORT?= yes	# network server

PROG=	isibootd
SRCS=	isibootd.c
MAN=	isibootd.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.own.mk>
.include <bsd.prog.mk>
