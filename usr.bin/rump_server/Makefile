#	$NetBSD: Makefile,v 1.3.4.1 2014/05/22 11:42:49 yamt Exp $
#

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		installed by ../rump_allserver

LDADD+= -Wl,--whole-archive -lrump -lrumpuser -Wl,--no-whole-archive -lpthread

.include <bsd.prog.mk>
