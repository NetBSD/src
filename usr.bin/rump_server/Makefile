#	$NetBSD: Makefile,v 1.2.2.1 2011/02/08 16:20:12 bouyer Exp $
#

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		installed by ../rump_allserver

LDADD+= -lrump -lrumpuser -lpthread -lutil

.include <bsd.prog.mk>
