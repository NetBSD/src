# $NetBSD: Makefile,v 1.1 2011/07/31 16:00:21 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	sdtemp
IOCONF=	sdtemp.ioconf
SRCS=	sdtemp.c

WARNS=	4

.include <bsd.kmodule.mk>
