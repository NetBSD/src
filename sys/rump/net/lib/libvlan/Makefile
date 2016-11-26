#	$NetBSD $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_vlan
COMMENT=IEEE 802.1q encapsulation

IOCONF=	VLAN.ioconf
SRCS=	if_vlan.c

SRCS+=	vlan_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
