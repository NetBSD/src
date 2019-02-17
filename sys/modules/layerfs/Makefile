#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:53 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/genfs

KMOD=	layerfs
SRCS=	layer_subr.c layer_vfsops.c layer_vnops.c

WARNS=	3

CWARNFLAGS.clang=	-Wno-conversion

.include <bsd.kmodule.mk>
