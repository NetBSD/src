$NetBSD: Makefile,v 1.4.2.2 2009/01/17 13:29:29 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/dm

KMOD=	dm
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
