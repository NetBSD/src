#	$NetBSD: Makefile,v 1.5 2013/09/10 18:28:46 pooka Exp $
#

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		installed by ../rump_allserver

LDADD+= -lrump -lrumpuser -lpthread

.include <bsd.own.mk>

.if ${RUMP_SERVER_LIBUTIL:Uyes} == "yes"
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

.include <bsd.prog.mk>
