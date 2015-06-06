# $NetBSD: Makefile,v 1.1.30.1 2015/06/06 14:40:24 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/iscsi

KMOD=	iscsi
SRCS=	base64.c iscsi_ioctl.c iscsi_main.c iscsi_rcv.c
SRCS+=	iscsi_send.c iscsi_text.c iscsi_utils.c

.include <bsd.kmodule.mk>
