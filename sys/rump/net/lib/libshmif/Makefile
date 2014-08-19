#	$NetBSD: Makefile,v 1.3.18.2 2014/08/20 00:04:43 tls Exp $
#

LIB=	rumpnet_shmif

SRCS=	if_shmem.c shmif_busops.c
SRCS+=	shmif_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpkern

RUMPCOMP_USER_SRCS=	shmif_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
