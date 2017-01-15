#	$NetBSD: Makefile,v 1.1.2.1 2017/01/15 20:27:34 bouyer Exp $
#

NOLINT=	#defined

.include <bsd.own.mk>

.PATH:	${.CURDIR}/../../../../netcan

LIB=	rumpnet_netcan
COMMENT=CAN (PF_CAN)

IOCONF=	NETCAN.ioconf

SRCS=	can.c can_pcb.c can_proto.c if_canloop.c

SRCS+=	netcan_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
