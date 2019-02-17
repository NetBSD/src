#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:56 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/opencrypto

KMOD=	opencrypto
IOCONF=	opencrypto.ioconf
SRCS=	criov.c crypto.c xform.c

WARNS=	3

.include <bsd.kmodule.mk>
