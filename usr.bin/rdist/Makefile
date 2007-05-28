#	$NetBSD: Makefile,v 1.11 2007/05/28 12:06:29 tls Exp $
#	from: @(#)Makefile	8.3 (Berkeley) 7/19/93

USE_FORT?= yes	# network client/server

CPPFLAGS+= -I${.CURDIR}
PROG=	rdist
SRCS=	docmd.c expand.c gram.y lookup.c main.c server.c

.include <bsd.prog.mk>
