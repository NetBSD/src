# $NetBSD: Makefile,v 1.2 2015/12/04 01:21:12 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sbin/gpt

TESTS_SH=	t_gpt

FILESDIR=       ${TESTSDIR}
FILES=          gpt.2disk.show.guid gpt.2part.show.normal gpt.empty.show.normal
FILES+=		gpt.backup

.include <bsd.test.mk>
