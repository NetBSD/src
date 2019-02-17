# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:53 rin Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_vlan
IOCONF=		vlan.ioconf
SRCS=		if_vlan.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

WARNS=		3

.include <bsd.kmodule.mk>
