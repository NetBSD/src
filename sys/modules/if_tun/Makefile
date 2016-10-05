# $NetBSD: Makefile,v 1.1.4.2 2016/10/05 20:56:05 skrll Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_tun
IOCONF=		tun.ioconf
SRCS=		if_tun.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
