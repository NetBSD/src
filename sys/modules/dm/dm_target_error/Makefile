#$NetBSD: Makefile,v 1.1.24.1 2012/02/18 07:35:36 mrg Exp $

.include "../../Makefile.inc"

S!=     cd ${.CURDIR}/../../..;pwd
.PATH:	${S}/dev/dm

CPPFLAGS += -DDM_TARGET_MODULE
KMOD=	dm_target_error
SRCS=   dm_target_error.c

.include <bsd.kmodule.mk>
