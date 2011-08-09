# $NetBSD: Makefile,v 1.4 2011/08/09 01:42:25 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/dtv

KMOD=	dtv
IOCONF=	dtv.ioconf
SRCS=	dtv_buffer.c \
	dtv_demux.c \
	dtv_device.c \
	dtv_ioctl.c \
	dtv_scatter.c

WARNS=	4

.include <bsd.kmodule.mk>
