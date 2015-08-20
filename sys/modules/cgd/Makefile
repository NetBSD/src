#	$NetBSD: Makefile,v 1.2 2015/08/20 11:05:00 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev

KMOD=	cgd
IOCONF=	cgd.ioconf
SRCS=	cgd.c cgd_crypto.c

.include <bsd.kmodule.mk>
