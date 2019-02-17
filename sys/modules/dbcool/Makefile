# $NetBSD: Makefile,v 1.3 2019/02/17 04:05:46 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	dbcool
IOCONF=	dbcool.ioconf
SRCS=	dbcool.c

WARNS=	3

.include <bsd.kmodule.mk>
