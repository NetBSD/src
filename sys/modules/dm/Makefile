$NetBSD: Makefile,v 1.1.2.1 2008/08/03 21:48:14 haad Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/dm

KMOD=	dm
SRCS=	device-mapper.c dm_dev.c dm_ioctl.c dm_pdev.c dm_table.c dm_target.c \
        dm_target_error.c dm_target_linear.c dm_target_snapshot.c \
        dm_target_zero.c

.include <bsd.kmodule.mk>
