#$NetBSD: Makefile,v 1.1.24.1 2012/02/18 07:35:37 mrg Exp $

.include "../../Makefile.inc"

S!=             cd ${.CURDIR}/../../..;pwd
.PATH:	${S}/dev/dm

CPPFLAGS += -DDM_TARGET_MODULE
KMOD=	dm_target_snapshot
SRCS=   dm_target_snapshot.c

.include <bsd.kmodule.mk>
