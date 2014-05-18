#	$NetBSD: Makefile,v 1.4.4.1 2014/05/18 17:46:20 rmind Exp $
#

LIB=	rumpnet_shmif

SRCS=	if_shmem.c shmif_busops.c
SRCS+=	shmif_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpkern

RUMPCOMP_USER_SRCS=	shmif_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
