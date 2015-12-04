# $NetBSD: Makefile,v 1.3 2015/12/04 01:42:47 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sbin/gpt

TESTS_SH=	t_gpt

FILESDIR=       ${TESTSDIR}
FILES=          gpt.2part.show.uuid gpt.2part.show.normal gpt.empty.show.normal
FILES+=		gpt.backup

.include <bsd.test.mk>
