#       $NetBSD: Makefile,v 1.1 2003/06/12 23:15:48 provos Exp $

NOMAN=		# defined

PROG=		eventtest
DPADD+=		${LIBSKEY}
LDADD+=		-levent

.PATH:		${.CURDIR}/..

regress: eventtest
	@echo Testing libevent
	./eventtest

.include <bsd.prog.mk>

