#	$NetBSD: Makefile,v 1.2 2015/08/20 11:05:01 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/opencrypto

KMOD=	opencrypto
IOCONF=	opencrypto.ioconf
SRCS=	criov.c crypto.c xform.c

.include <bsd.kmodule.mk>
