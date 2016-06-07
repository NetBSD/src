#	$NetBSD: Makefile,v 1.4 2016/06/07 01:06:28 pgoyette Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/i2c

KMOD=   	iic
IOCONF=		iic.ioconf
SRCS=		i2c.c

.include <bsd.kmodule.mk>
