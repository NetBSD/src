#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:44 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev

KMOD=	cgd
IOCONF=	cgd.ioconf
SRCS=	cgd.c cgd_crypto.c

WARNS=	3

.include <bsd.kmodule.mk>
