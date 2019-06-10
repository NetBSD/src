# $NetBSD: Makefile,v 1.4.18.1 2019/06/10 22:09:30 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/iscsi

KMOD=	iscsi
IOCONF=	iscsi.ioconf
SRCS=	base64.c iscsi_ioctl.c iscsi_main.c iscsi_rcv.c
SRCS+=	iscsi_send.c iscsi_text.c iscsi_utils.c

WARNS=	3

.include <bsd.kmodule.mk>
