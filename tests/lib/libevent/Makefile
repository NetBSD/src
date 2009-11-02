# $NetBSD: Makefile,v 1.1 2009/11/02 10:15:45 plunky Exp $

TESTSDIR=	${TESTSBASE}/lib/libevent

TESTS_SH=	t_event

.include <bsd.own.mk>
.include <../../../external/bsd/libevent/Makefile.inc>

.PATH:		${LIBEVENT_DIR}/dist/test

BINDIR=		${TESTSDIR}

PROG=		h_event
MAN=		# defined
SRCS=		regress.c regress.gen.c regress_dns.c regress_http.c \
		regress_rpc.c

DPADD+=		${LIBEVENT}
LDADD+=		-levent

CPPFLAGS.regress.c+=		-Wno-cast-qual -Wno-sign-compare -Wno-shadow
CPPFLAGS.regress_http.c+=	-Wno-cast-qual -Wno-sign-compare -Wno-shadow
CPPFLAGS.regress_rpc.c+=	-Wno-cast-qual -Wno-shadow

.include <bsd.test.mk>
