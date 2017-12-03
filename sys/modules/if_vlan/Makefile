# $NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:56 jdolecek Exp $

.include "../Makefile.inc"

.PATH:  ${S}/net

KMOD=		if_vlan
IOCONF=		vlan.ioconf
SRCS=		if_vlan.c

CPPFLAGS+=	-DINET
CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
