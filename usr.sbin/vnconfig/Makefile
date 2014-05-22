#	$NetBSD: Makefile,v 1.6.54.1 2014/05/22 11:43:11 yamt Exp $

PROG=		vnconfig
MAN=		vnconfig.8
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
LINKS+=		${BINDIR}/vnconfig ${BINDIR}/vndconfig
MLINKS+=	vnconfig.8 vndconfig.8

.include <bsd.prog.mk>
