# $NetBSD: Makefile,v 1.1.2.2 2016/09/14 03:04:19 pgoyette Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_vlan
IOCONF=		vlan.ioconf
SRCS=		if_vlan.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
