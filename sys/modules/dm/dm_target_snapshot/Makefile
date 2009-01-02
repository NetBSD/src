#$NetBSD: Makefile,v 1.1 2009/01/02 01:06:16 haad Exp $

.include "../../Makefile.inc"

S!=             cd ${.CURDIR}/../../..;pwd
.PATH:	${S}/dev/dm

CFLAGS += -DDM_TARGET_MODULE
KMOD=	dm_target_snapshot
SRCS=   dm_target_snapshot.c

.include <bsd.kmodule.mk>
