#	$NetBSD: Makefile,v 1.3.10.1 2014/08/20 00:05:03 tls Exp $
#

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		installed by ../rump_allserver

LDADD+= -Wl,--whole-archive -lrump -lrumpuser -Wl,--no-whole-archive -lpthread

.include <bsd.prog.mk>
