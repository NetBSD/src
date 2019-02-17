#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:48 rin Exp $

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
