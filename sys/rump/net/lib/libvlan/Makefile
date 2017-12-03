#	$NetBSD: Makefile,v 1.2.18.2 2017/12/03 11:39:19 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_vlan
COMMENT=IEEE 802.1q encapsulation

IOCONF=	VLAN.ioconf
SRCS=	if_vlan.c

SRCS+=	vlan_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
