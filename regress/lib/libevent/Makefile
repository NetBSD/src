#       $NetBSD: Makefile,v 1.3.30.1 2008/05/18 12:30:47 yamt Exp $

NOMAN=	# defined

CPPFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR}/../../../lib/libevent

SRCS=	regress.c regress.gen.c regress_dns.c regress_http.c regress_rpc.c
PROG=	eventtest
DPADD+=	${LIBEVENT}
LDADD+=	-levent

.PATH:	${.CURDIR}/..

regress: eventtest
	@echo Testing libevent
	@EVENT_SHOW_METHOD="yes" EVENT_NOPOLL="yes" ./eventtest
	@echo
	@EVENT_SHOW_METHOD="yes" EVENT_NOKQUEUE="yes" ./eventtest

.include <bsd.prog.mk>

