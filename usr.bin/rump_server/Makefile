#	$NetBSD: Makefile,v 1.2 2010/12/13 19:39:37 pooka Exp $
#

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		installed by ../rump_allserver

LDADD+= -lrump -lrumpuser -lpthread

.include <bsd.prog.mk>
