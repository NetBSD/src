
.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_pppoe
COMPONENT= PPP over Ethernet protocol network interface

IOCONF=	PPPOE.ioconf
SRCS=	if_pppoe.c

SRCS+=	pppoe_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
