#	$NetBSD: Makefile,v 1.1 2011/10/02 11:43:30 mbalmer Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/i2c

KMOD=   	iic
IOCONF=		i2c.ioconf
SRCS=		i2c.c i2c_bitbang.c

CPPFLAGS+=	-I${S}/i2c

.include <bsd.kmodule.mk>
