#	$NetBSD: Makefile,v 1.2.10.2 2017/12/03 11:39:18 jdolecek Exp $
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
