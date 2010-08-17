#	$NetBSD: Makefile,v 1.1.8.1 2010/08/17 06:48:05 uebayasi Exp $
#

LIB=	rumpnet_shmif

SRCS=	if_shmem.c shmif_busops.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
