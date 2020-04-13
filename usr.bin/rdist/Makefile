#	$NetBSD: Makefile,v 1.11.76.2 2020/04/13 08:05:46 martin Exp $
#	from: @(#)Makefile	8.3 (Berkeley) 7/19/93

USE_FORT?= yes	# network client/server

CPPFLAGS+= -I${.CURDIR}
PROG=	rdist
SRCS=	docmd.c expand.c gram.y lookup.c main.c server.c

.include <bsd.own.mk>

COPTS.server.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
