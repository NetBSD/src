#	$NetBSD: Makefile,v 1.15 2023/06/03 09:09:18 lukem Exp $
#	from: @(#)Makefile	8.3 (Berkeley) 7/19/93

USE_FORT?= yes	# network client/server

CPPFLAGS+= -I${.CURDIR}
PROG=	rdist
SRCS=	docmd.c expand.c gram.y lookup.c main.c server.c

.include <bsd.own.mk>

COPTS.server.c+=	${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
