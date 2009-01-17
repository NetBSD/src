#$NetBSD: Makefile,v 1.1.2.2 2009/01/17 13:29:29 mjf Exp $

.include "../../Makefile.inc"

S!=     cd ${.CURDIR}/../../..;pwd
.PATH:	${S}/dev/dm

CFLAGS += -DDM_TARGET_MODULE
KMOD=	dm_target_error
SRCS=   dm_target_error.c

.include <bsd.kmodule.mk>
