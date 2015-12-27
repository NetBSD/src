#	$NetBSD: Makefile,v 1.6.6.1 2015/12/27 12:10:18 skrll Exp $
#

LIB=	rumpnet_shmif
COMMENT=Shared memory bus network interface

SRCS=	if_shmem.c shmif_busops.c
SRCS+=	shmif_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpkern

RUMPCOMP_USER_SRCS=	shmif_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
