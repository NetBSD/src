# $NetBSD: Makefile,v 1.1 2016/08/07 14:27:39 christos Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_tun
IOCONF=		tun.ioconf
SRCS=		if_tun.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
