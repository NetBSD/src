#$NetBSD: Makefile,v 1.1 2009/01/02 11:03:24 haad Exp $

.include "../../Makefile.inc"

S!=     cd ${.CURDIR}/../../..;pwd
.PATH:	${S}/dev/dm

CFLAGS += -DDM_TARGET_MODULE
KMOD=	dm_target_mirror
SRCS=   dm_target_mirror.c

.include <bsd.kmodule.mk>
