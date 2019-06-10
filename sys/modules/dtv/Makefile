# $NetBSD: Makefile,v 1.4.54.1 2019/06/10 22:09:23 christos Exp $

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
