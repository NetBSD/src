#	$NetBSD: Makefile,v 1.1.54.1 2019/06/10 22:09:23 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	emdtv
IOCONF=	emdtv.ioconf
SRCS=	emdtv.c
SRCS+=	emdtv_board.c
SRCS+=	emdtv_dtv.c
SRCS+=	emdtv_i2c.c
SRCS+=	emdtv_ir.c

.include <bsd.kmodule.mk>
