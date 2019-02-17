# $NetBSD: Makefile,v 1.5 2019/02/17 04:05:53 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/iscsi

KMOD=	iscsi
IOCONF=	iscsi.ioconf
SRCS=	base64.c iscsi_ioctl.c iscsi_main.c iscsi_rcv.c
SRCS+=	iscsi_send.c iscsi_text.c iscsi_utils.c

WARNS=	3

.include <bsd.kmodule.mk>
