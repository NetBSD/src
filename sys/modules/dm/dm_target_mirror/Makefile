#$NetBSD: Makefile,v 1.1.4.2 2009/01/19 13:20:08 skrll Exp $

.include "../../Makefile.inc"

S!=     cd ${.CURDIR}/../../..;pwd
.PATH:	${S}/dev/dm

CFLAGS += -DDM_TARGET_MODULE
KMOD=	dm_target_mirror
SRCS=   dm_target_mirror.c

.include <bsd.kmodule.mk>
