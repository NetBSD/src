#	$NetBSD: Makefile,v 1.2 2010/08/12 21:41:47 pooka Exp $
#

LIB=	rumpnet_shmif

SRCS=	if_shmem.c shmif_busops.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
