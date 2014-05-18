# $NetBSD: Makefile,v 1.1.2.2 2014/05/18 17:46:11 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	hythygtemp
IOCONF=	hythygtemp.ioconf
SRCS=	hytp14.c

.include <bsd.kmodule.mk>
