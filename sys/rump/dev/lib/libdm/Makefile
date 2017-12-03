#	$NetBSD: Makefile,v 1.3.18.2 2017/12/03 11:39:07 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../dev/dm

LIB=	rumpdev_dm
COMMENT=Device-mapper driver (for LVM)

IOCONF=	DM.ioconf
SRCS=   device-mapper.c dm_dev.c dm_ioctl.c dm_pdev.c dm_table.c dm_target.c \
        dm_target_linear.c dm_target_stripe.c

SRCS+=	dm_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
