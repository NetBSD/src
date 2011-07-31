# $NetBSD: Makefile,v 1.1 2011/07/31 16:05:28 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	dbcool
IOCONF=	dbcool.ioconf
SRCS=	dbcool.c

WARNS=	3

.include <bsd.kmodule.mk>
