# $NetBSD: Makefile,v 1.1.8.2 2014/08/20 00:04:33 tls Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	hythygtemp
IOCONF=	hythygtemp.ioconf
SRCS=	hytp14.c

.include <bsd.kmodule.mk>
