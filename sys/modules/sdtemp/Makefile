# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:57 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	sdtemp
IOCONF=	sdtemp.ioconf
SRCS=	sdtemp.c

.include <bsd.kmodule.mk>
