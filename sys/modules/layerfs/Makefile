#	$NetBSD: Makefile,v 1.2 2011/05/26 12:56:31 joerg Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/genfs

KMOD=	layerfs
SRCS=	layer_subr.c layer_vfsops.c layer_vnops.c

CWARNFLAGS.clang=	-Wno-conversion

.include <bsd.kmodule.mk>
