#	$NetBSD: Makefile,v 1.3.8.1 2014/05/22 11:41:17 yamt Exp $
#

LIB=	rumpnet_shmif

SRCS=	if_shmem.c shmif_busops.c
SRCS+=	shmif_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpkern

RUMPCOMP_USER_SRCS=	shmif_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
