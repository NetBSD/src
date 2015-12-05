# $NetBSD: Makefile,v 1.7 2015/12/05 18:45:35 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sbin/gpt

TESTS_SH=	t_gpt

FILESDIR=       ${TESTSDIR}
FILES=		gpt.empty.show.normal gpt.backup gpt.removepart.show.normal
FILES+=		gpt.2part.show.uuid gpt.2part.show.normal gpt.2part.show.label
FILES+=		gpt.resizedisk.show.normal gpt.resizepart.show.normal
FILES+=		disklabel gpt.disklabel

.include <bsd.test.mk>
