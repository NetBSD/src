#	$NetBSD: Makefile,v 1.13 2019/06/01 06:59:18 kre Exp $
#

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		installed by ../rump_allserver

LDADD+=		-Wl,--whole-archive -lrumpkern_sysproxy -lrump -lrumpvfs \
		-lrumpuser -Wl,--no-whole-archive -lpthread

.include <bsd.prog.mk>
