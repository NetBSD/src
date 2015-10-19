#	$NetBSD: Makefile,v 1.3 2015/10/19 16:16:35 pooka Exp $
#

.PATH:  ${.CURDIR}/../../../../miscfs/genfs

LIB=	rumpvfs_layerfs
COMMENT=Layer file system support (used by other drivers)

SRCS=	layer_subr.c layer_vfsops.c layer_vnops.c

CWARNFLAGS.clang+=	-Wno-conversion

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
