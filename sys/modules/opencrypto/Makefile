#	$NetBSD: Makefile,v 1.2.18.1 2019/06/10 22:09:36 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/opencrypto

KMOD=	opencrypto
IOCONF=	opencrypto.ioconf
SRCS=	criov.c crypto.c xform.c

WARNS=	3

.include <bsd.kmodule.mk>
