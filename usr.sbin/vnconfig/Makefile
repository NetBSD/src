#	$NetBSD: Makefile,v 1.6.60.1 2013/06/23 06:29:06 tls Exp $

PROG=		vnconfig
MAN=		vnconfig.8
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
LINKS+=		${BINDIR}/vnconfig ${BINDIR}/vndconfig
MLINKS+=	vnconfig.8 vndconfig.8

.include <bsd.prog.mk>
