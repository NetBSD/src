# $NetBSD: Makefile,v 1.1.30.2 2015/09/22 12:06:09 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/iscsi

KMOD=	iscsi
IOCONF=	iscsi.ioconf
SRCS=	base64.c iscsi_ioctl.c iscsi_main.c iscsi_rcv.c
SRCS+=	iscsi_send.c iscsi_text.c iscsi_utils.c

.include <bsd.kmodule.mk>
