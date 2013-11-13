#	$NetBSD: Makefile,v 1.7 2013/11/13 17:47:27 pooka Exp $
#

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		installed by ../rump_allserver

LDADD+= -Wl,--whole-archive -lrump -lrumpuser -Wl,--no-whole-archive -lpthread

.include <bsd.prog.mk>
