# $NetBSD: Makefile,v 1.1 2014/05/18 11:46:24 kardel Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	hythygtemp
IOCONF=	hythygtemp.ioconf
SRCS=	hytp14.c

.include <bsd.kmodule.mk>
