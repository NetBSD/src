#$NetBSD: Makefile,v 1.1.20.1 2012/04/17 00:08:36 yamt Exp $

.include "../../Makefile.inc"

S!=             cd ${.CURDIR}/../../..;pwd
.PATH:	${S}/dev/dm

CPPFLAGS += -DDM_TARGET_MODULE
KMOD=	dm_target_snapshot
SRCS=   dm_target_snapshot.c

.include <bsd.kmodule.mk>
