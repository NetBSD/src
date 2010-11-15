#	$NetBSD: Makefile,v 1.3 2010/11/15 22:48:06 pooka Exp $
#

LIB=	rumpnet_shmif

SRCS=	if_shmem.c shmif_busops.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
