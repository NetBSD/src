#	$NetBSD: Makefile,v 1.4 2013/09/10 18:22:55 pooka Exp $
#

.PATH: ${.CURDIR}/../rump_allserver

PROG=		rump_server
SRCS=		rump_allserver.c
NOMAN=		installed by ../rump_allserver

LDADD+= -lrump -lrumpuser -lpthread

.if ${RUMP_SERVER_LIBUTIL:Uyes} == "yes"
DPADD+=	${LIBUTIL}
LDADD+=	-lutil
.endif

.include <bsd.prog.mk>
