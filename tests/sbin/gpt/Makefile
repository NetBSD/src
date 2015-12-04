# $NetBSD: Makefile,v 1.6 2015/12/04 20:16:23 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sbin/gpt

TESTS_SH=	t_gpt

FILESDIR=       ${TESTSDIR}
FILES=		gpt.empty.show.normal gpt.backup gpt.removepart.show.normal
FILES+=          gpt.2part.show.uuid gpt.2part.show.normal gpt.2part.show.label
FILES+=		gpt.resizedisk.show.normal gpt.resizepart.show.normal

.include <bsd.test.mk>
