#$NetBSD: Makefile,v 1.2 2012/02/05 18:46:04 christos Exp $

.include "../../Makefile.inc"

S!=     cd ${.CURDIR}/../../..;pwd
.PATH:	${S}/dev/dm

CPPFLAGS += -DDM_TARGET_MODULE
KMOD=	dm_target_error
SRCS=   dm_target_error.c

.include <bsd.kmodule.mk>
