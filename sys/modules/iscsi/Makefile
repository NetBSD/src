# $NetBSD: Makefile,v 1.1.8.2 2012/04/17 00:08:36 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/iscsi

KMOD=	iscsi
SRCS=	base64.c iscsi_ioctl.c iscsi_main.c iscsi_profile.c iscsi_rcv.c
SRCS+=	iscsi_send.c iscsi_test.c iscsi_text.c iscsi_utils.c

.include <bsd.kmodule.mk>
