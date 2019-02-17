#	$NetBSD: Makefile,v 1.6 2019/02/17 04:05:53 rin Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/i2c

KMOD=   	iic
IOCONF=		iic.ioconf
SRCS=		i2c.c i2c_subr.c

WARNS=		3

.include <bsd.kmodule.mk>
