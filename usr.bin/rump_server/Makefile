#	$NetBSD: Makefile,v 1.9 2016/01/23 21:22:50 christos Exp $
#

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		installed by ../rump_allserver

CPPFLAGS+=	-D_KERNTYPES
LDADD+=		-Wl,--whole-archive -lrumpkern_sysproxy -lrump \
		-lrumpuser -Wl,--no-whole-archive -lpthread

.include <bsd.prog.mk>
