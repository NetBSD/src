#	$NetBSD: Makefile,v 1.1.2.1 2010/04/30 14:44:23 uebayasi Exp $
#

.PATH:	${.CURDIR}/../../../../dev/dm

LIB=	rumpdev_dm

SRCS=   device-mapper.c dm_dev.c dm_ioctl.c dm_pdev.c dm_table.c dm_target.c \
        dm_target_linear.c dm_target_stripe.c

SRCS+=	component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

LDADD+=         -lrumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
