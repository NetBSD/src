#	$NetBSD: Makefile,v 1.2.56.1 2019/06/10 22:09:31 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/genfs

KMOD=	layerfs
SRCS=	layer_subr.c layer_vfsops.c layer_vnops.c

WARNS=	3

CWARNFLAGS.clang=	-Wno-conversion

.include <bsd.kmodule.mk>
