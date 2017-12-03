#	$NetBSD: Makefile,v 1.1.24.1 2017/12/03 11:38:50 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev

KMOD=	cgd
IOCONF=	cgd.ioconf
SRCS=	cgd.c cgd_crypto.c

.include <bsd.kmodule.mk>
