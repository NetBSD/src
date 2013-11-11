#	$NetBSD: Makefile,v 1.6 2013/11/11 15:53:56 pooka Exp $
#

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		installed by ../rump_allserver

LDADD+= -Wl,--whole-archive -lrump -lrumpuser -Wl,--no-whole-archive -lpthread

.include <bsd.own.mk>

.if ${RUMP_SERVER_LIBUTIL:Uyes} == "yes"
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

.include <bsd.prog.mk>
