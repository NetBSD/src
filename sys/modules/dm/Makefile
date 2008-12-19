$NetBSD: Makefile,v 1.2 2008/12/19 15:36:58 haad Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/dm

KMOD=	dm
SRCS=	device-mapper.c dm_dev.c dm_ioctl.c dm_pdev.c dm_table.c dm_target.c \
        dm_target_error.c dm_target_linear.c dm_target_snapshot.c \
        dm_target_zero.c

.include <bsd.kmodule.mk>
