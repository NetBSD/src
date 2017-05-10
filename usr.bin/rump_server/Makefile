#	$NetBSD: Makefile,v 1.10 2017/05/10 01:11:52 sevan Exp $
#

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		installed by ../rump_allserver

LDADD+=		-Wl,--whole-archive -lrumpkern_sysproxy -lrump \
		-lrumpuser -Wl,--no-whole-archive -lpthread

.include <bsd.prog.mk>
