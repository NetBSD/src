# $NetBSD: Makefile,v 1.3.4.2 2008/01/09 01:59:37 matt Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/util/id

TESTS_SH=	t_groups
TESTS_SH+=	t_id
TESTS_SH+=	t_whoami

BINDIR=		${TESTSDIR}
PROG=		h_id
.PATH:  	${NETBSDSRCDIR}/usr.bin/id
SRCS=		id.c pwgr.c

.include <bsd.test.mk>
.include <bsd.subdir.mk>
