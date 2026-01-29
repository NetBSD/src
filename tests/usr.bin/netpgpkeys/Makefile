# $NetBSD: Makefile,v 1.2 2026/01/29 16:14:45 wiz Exp $

.include <bsd.own.mk>

TESTSDIR= ${TESTSBASE}/usr.bin/netpgpkeys

TESTS_SH+= t_netpgpkeys

FILESDIR=	${TESTSDIR}/data
FILES+=		data/testkey-ec.pub
FILES+=		data/testkey-rsa.pub

.include <bsd.test.mk>
