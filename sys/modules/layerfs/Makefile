#	$NetBSD: Makefile,v 1.1.16.1 2011/06/06 09:09:49 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/genfs

KMOD=	layerfs
SRCS=	layer_subr.c layer_vfsops.c layer_vnops.c

CWARNFLAGS.clang=	-Wno-conversion

.include <bsd.kmodule.mk>
