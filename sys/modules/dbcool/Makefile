# $NetBSD: Makefile,v 1.2.54.1 2019/06/10 22:09:20 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	dbcool
IOCONF=	dbcool.ioconf
SRCS=	dbcool.c

WARNS=	3

.include <bsd.kmodule.mk>
