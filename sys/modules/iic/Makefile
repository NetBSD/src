#	$NetBSD: Makefile,v 1.7 2022/04/01 15:49:11 pgoyette Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/i2c

KMOD=   	iic
IOCONF=		iic.ioconf
SRCS=		i2c.c

WARNS=		3

.include <bsd.kmodule.mk>
