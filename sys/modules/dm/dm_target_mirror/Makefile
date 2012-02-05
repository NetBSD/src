#$NetBSD: Makefile,v 1.2 2012/02/05 18:46:05 christos Exp $

.include "../../Makefile.inc"

S!=     cd ${.CURDIR}/../../..;pwd
.PATH:	${S}/dev/dm

CPPFLAGS += -DDM_TARGET_MODULE
KMOD=	dm_target_mirror
SRCS=   dm_target_mirror.c

.include <bsd.kmodule.mk>
