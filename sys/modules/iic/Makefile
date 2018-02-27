#	$NetBSD: Makefile,v 1.5 2018/02/27 00:15:34 pgoyette Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/i2c

KMOD=   	iic
IOCONF=		iic.ioconf
SRCS=		i2c.c i2c_subr.c

.include <bsd.kmodule.mk>
