# $NetBSD: Makefile,v 1.6 2015/08/20 11:05:01 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/dm

KMOD=	dm
IOCONF=	dm.ioconf
SRCS=	device-mapper.c dm_dev.c dm_ioctl.c dm_pdev.c dm_table.c dm_target.c \
        dm_target_linear.c dm_target_stripe.c

# Do not build unneeded modules now.
MK_DM_TARGETS= no

.if (${MK_DM_TARGETS} != "no")
SUBDIR+= dm_target_error
SUBDIR+= dm_target_mirror
SUBDIR+= dm_target_snapshot
SUBDIR+= dm_target_zero
.endif

.include <bsd.kmodule.mk>
.include <bsd.subdir.mk>
