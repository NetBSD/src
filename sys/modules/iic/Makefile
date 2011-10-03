#	$NetBSD: Makefile,v 1.3 2011/10/03 22:27:23 jmcneill Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/i2c

KMOD=   	iic
IOCONF=		iic.ioconf
SRCS=		i2c.c i2c_bitbang.c i2c_subr.c

.include <bsd.kmodule.mk>
