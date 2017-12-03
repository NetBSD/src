#	$NetBSD: Makefile,v 1.3.18.3 2017/12/03 11:39:19 jdolecek Exp $
#

LIB=	rumpnet_shmif
COMMENT=Shared memory bus network interface

SRCS=	if_shmem.c shmif_busops.c
SRCS+=	shmif_component.c

RUMPCOMP_USER_SRCS=	shmif_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
