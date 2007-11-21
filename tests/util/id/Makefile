# $NetBSD: Makefile,v 1.3 2007/11/21 15:39:34 jmmv Exp $

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
