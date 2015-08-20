# $NetBSD: Makefile,v 1.4 2015/08/20 11:05:01 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/iscsi

KMOD=	iscsi
IOCONF=	iscsi.ioconf
SRCS=	base64.c iscsi_ioctl.c iscsi_main.c iscsi_rcv.c
SRCS+=	iscsi_send.c iscsi_text.c iscsi_utils.c

.include <bsd.kmodule.mk>
