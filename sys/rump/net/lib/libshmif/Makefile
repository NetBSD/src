#	$NetBSD: Makefile,v 1.1.4.3 2010/10/09 03:32:44 yamt Exp $
#

LIB=	rumpnet_shmif

SRCS=	if_shmem.c shmif_busops.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
