#	$NetBSD: Makefile,v 1.2.4.2 2017/01/07 08:56:53 pgoyette Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_vlan
COMMENT=IEEE 802.1q encapsulation

IOCONF=	VLAN.ioconf
SRCS=	if_vlan.c

SRCS+=	vlan_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
