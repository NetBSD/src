# $NetBSD: Makefile,v 1.8.28.1 2026/11/04 23:28:21 perseant Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sbin/gpt

TESTS_SH=	t_gpt

FILESDIR=       ${TESTSDIR}
FILES=		gpt.empty.show.normal gpt.backup gpt.removepart.show.normal
FILES+=		gpt.2part.show.uuid gpt.2part.show.normal gpt.2part.show.label
FILES+=		gpt.resizedisk.show.normal gpt.resizepart.show.normal
FILES+=		gpt.disklabel gpt.disklabel.show.normal gpt.empty.show.clean

FILES+=		gpt.2part.show.label-p gpt.2part.show.normal-p
FILES+=		gpt.2part.show.uuid-p gpt.disklabel.show.normal-p
FILES+=		gpt.empty.show.clean-p gpt.empty.show.normal-p
FILES+=		gpt.removepart.show.normal-p gpt.resizedisk.show.normal-p
FILES+=		gpt.resizepart.show.normal-p

.include <bsd.test.mk>
