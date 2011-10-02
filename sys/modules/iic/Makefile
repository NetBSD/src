#	$NetBSD: Makefile,v 1.2 2011/10/02 21:14:22 jmcneill Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/i2c

KMOD=   	iic
IOCONF=		iic.ioconf
SRCS=		i2c.c i2c_bitbang.c

.include <bsd.kmodule.mk>
