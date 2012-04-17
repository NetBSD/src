# $NetBSD: Makefile,v 1.1.2.2 2012/04/17 00:09:19 yamt Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/id

TESTS_SH=	t_groups
TESTS_SH+=	t_id
TESTS_SH+=	t_whoami

BINDIR=		${TESTSDIR}
PROG=		h_id
.PATH:  	${NETBSDSRCDIR}/usr.bin/id
SRCS=		id.c pwgr.c

COPTS.id.c += -Wno-format-nonliteral

.include <bsd.test.mk>
