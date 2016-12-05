#	$NetBSD: Makefile,v 1.2.2.2 2016/12/05 10:55:29 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_vlan
COMMENT=IEEE 802.1q encapsulation

IOCONF=	VLAN.ioconf
SRCS=	if_vlan.c

SRCS+=	vlan_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
