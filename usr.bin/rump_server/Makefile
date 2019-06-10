#	$NetBSD: Makefile,v 1.12.10.1 2019/06/10 22:10:23 christos Exp $
#

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		installed by ../rump_allserver

LDADD+=		-Wl,--whole-archive -lrumpkern_sysproxy -lrump -lrumpvfs \
		-lrumpuser -Wl,--no-whole-archive -lpthread

.include <bsd.prog.mk>
