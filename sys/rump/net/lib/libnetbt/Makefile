#	$NetBSD: Makefile,v 1.1.24.2 2017/12/03 11:39:18 jdolecek Exp $
#

NOLINT=	#defined

.include <bsd.own.mk>

.PATH:	${.CURDIR}/../../../../netbt

LIB=	rumpnet_netbt
COMMENT=BlueTooth (PF_BLUETOOTH)

SRCS=	bt_proto.c bt_sysctl.c						\
	hci_event.c hci_ioctl.c hci_link.c hci_misc.c hci_socket.c	\
	hci_unit.c							\
	l2cap_lower.c l2cap_misc.c l2cap_signal.c l2cap_socket.c	\
	l2cap_upper.c							\
	rfcomm_dlc.c rfcomm_session.c rfcomm_socket.c rfcomm_upper.c	\
	sco_socket.c sco_upper.c

SRCS+=	netbt_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
