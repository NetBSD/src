#	$NetBSD: Makefile,v 1.1 2010/12/13 13:29:15 pooka Exp $
#

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		for now

LDADD+= -lrump -lrumpuser -lpthread

.include <bsd.prog.mk>
