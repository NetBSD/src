# $NetBSD: Makefile,v 1.1.4.2 2014/08/10 06:56:10 tls Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	hythygtemp
IOCONF=	hythygtemp.ioconf
SRCS=	hytp14.c

.include <bsd.kmodule.mk>
