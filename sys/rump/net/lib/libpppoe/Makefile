
.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_pppoe
COMPONENT= PPP over Ethernet protocol network interface

IOCONF=	PPPOE.ioconf
SRCS=	if_pppoe.c if_spppsubr.c

SRCS+=	pppoe_component.c

.ifdef RUMP_DEBUG
CPPFLAGS.if_pppoe.c+=	-DPPPOE_DEBUG
.endif

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
