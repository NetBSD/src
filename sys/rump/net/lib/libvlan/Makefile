#	$NetBSD: Makefile,v 1.2 2016/11/26 11:20:09 njoly Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_vlan
COMMENT=IEEE 802.1q encapsulation

IOCONF=	VLAN.ioconf
SRCS=	if_vlan.c

SRCS+=	vlan_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
