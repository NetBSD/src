# $NetBSD: Makefile,v 1.1.12.1 2017/12/03 11:38:56 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/iscsi

KMOD=	iscsi
IOCONF=	iscsi.ioconf
SRCS=	base64.c iscsi_ioctl.c iscsi_main.c iscsi_rcv.c
SRCS+=	iscsi_send.c iscsi_text.c iscsi_utils.c

.include <bsd.kmodule.mk>
