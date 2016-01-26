#	$NetBSD: Makefile,v 1.8 2016/01/26 23:12:19 pooka Exp $
#

LIB=	rumpnet_shmif
COMMENT=Shared memory bus network interface

SRCS=	if_shmem.c shmif_busops.c
SRCS+=	shmif_component.c

RUMPCOMP_USER_SRCS=	shmif_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
