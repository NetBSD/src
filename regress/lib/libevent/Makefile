#       $NetBSD: Makefile,v 1.2 2003/06/12 23:30:58 provos Exp $

NOMAN=		# defined

PROG=		eventtest
DPADD+=		${LIBSKEY}
LDADD+=		-levent

.PATH:		${.CURDIR}/..

regress: eventtest
	@echo Testing libevent
	@EVENT_SHOW_METHOD="yes" EVENT_NOPOLL="yes" ./eventtest
	@echo
	@EVENT_SHOW_METHOD="yes" EVENT_NOKQUEUE="yes" ./eventtest

.include <bsd.prog.mk>

