#	$NetBSD: Makefile,v 1.6 2015/10/19 16:16:32 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/dm

LIB=	rumpdev_dm
COMMENT=Device-mapper driver (for LVM)

IOCONF=	DM.ioconf
SRCS=   device-mapper.c dm_dev.c dm_ioctl.c dm_pdev.c dm_table.c dm_target.c \
        dm_target_linear.c dm_target_stripe.c

SRCS+=	dm_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
