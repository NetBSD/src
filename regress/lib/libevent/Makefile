#       $NetBSD: Makefile,v 1.3 2003/06/13 18:53:00 provos Exp $

NOMAN=		# defined

PROG=		eventtest
DPADD+=		${LIBEVENT}
LDADD+=		-levent

.PATH:		${.CURDIR}/..

regress: eventtest
	@echo Testing libevent
	@EVENT_SHOW_METHOD="yes" EVENT_NOPOLL="yes" ./eventtest
	@echo
	@EVENT_SHOW_METHOD="yes" EVENT_NOKQUEUE="yes" ./eventtest

.include <bsd.prog.mk>

