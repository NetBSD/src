#	$NetBSD: Makefile,v 1.1.12.1 2015/09/22 12:06:10 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/opencrypto

KMOD=	opencrypto
IOCONF=	opencrypto.ioconf
SRCS=	criov.c crypto.c xform.c

.include <bsd.kmodule.mk>
