# $NetBSD: Makefile,v 1.4 2015/12/04 16:59:39 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sbin/gpt

TESTS_SH=	t_gpt

FILESDIR=       ${TESTSDIR}
FILES=          gpt.2part.show.uuid gpt.2part.show.normal gpt.empty.show.normal
FILES+=		gpt.backup gpt.resizedisk.show.normal gpt.resizepart.show.normal

.include <bsd.test.mk>
