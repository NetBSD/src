#	$NetBSD: Makefile,v 1.12 2019/02/04 03:58:41 mrg Exp $
#	from: @(#)Makefile	8.3 (Berkeley) 7/19/93

USE_FORT?= yes	# network client/server

CPPFLAGS+= -I${.CURDIR}
PROG=	rdist
SRCS=	docmd.c expand.c gram.y lookup.c main.c server.c

.include <bsd.own.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.server.c += -Wno-error=format-truncation
.endif

.include <bsd.prog.mk>
