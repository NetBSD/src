#	$NetBSD: Makefile,v 1.1.10.3 2017/12/03 11:38:58 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/opencrypto

KMOD=	opencrypto
IOCONF=	opencrypto.ioconf
SRCS=	criov.c crypto.c xform.c

.include <bsd.kmodule.mk>
