# $NetBSD: Makefile,v 1.5 2019/02/17 04:05:48 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/dtv

KMOD=	dtv
IOCONF=	dtv.ioconf
SRCS=	dtv_buffer.c \
	dtv_demux.c \
	dtv_device.c \
	dtv_ioctl.c \
	dtv_scatter.c

.include <bsd.kmodule.mk>
