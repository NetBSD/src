#	$NetBSD: Makefile,v 1.5.4.1 2019/06/10 22:09:30 christos Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/i2c

KMOD=   	iic
IOCONF=		iic.ioconf
SRCS=		i2c.c i2c_subr.c

WARNS=		3

.include <bsd.kmodule.mk>
