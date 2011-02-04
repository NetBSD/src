#	$NetBSD: Makefile,v 1.3 2011/02/04 20:06:23 pooka Exp $
#

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		installed by ../rump_allserver

LDADD+= -lrump -lrumpuser -lpthread -lutil

.include <bsd.prog.mk>
