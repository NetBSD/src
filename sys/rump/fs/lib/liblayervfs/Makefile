#	$NetBSD: Makefile,v 1.2.32.1 2015/12/27 12:10:13 skrll Exp $
#

.PATH:  ${.CURDIR}/../../../../miscfs/genfs

LIB=	rumpvfs_layerfs
COMMENT=Layer file system support (used by other drivers)

SRCS=	layer_subr.c layer_vfsops.c layer_vnops.c

CWARNFLAGS.clang+=	-Wno-conversion

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
