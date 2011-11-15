# $NetBSD: Makefile,v 1.1 2011/11/15 16:50:46 riz Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/iscsi

KMOD=	iscsi
SRCS=	base64.c iscsi_ioctl.c iscsi_main.c iscsi_profile.c iscsi_rcv.c
SRCS+=	iscsi_send.c iscsi_test.c iscsi_text.c iscsi_utils.c

.include <bsd.kmodule.mk>
