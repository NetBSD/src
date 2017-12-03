#	$NetBSD: Makefile,v 1.2.14.1 2017/12/03 11:39:12 jdolecek Exp $
#

.PATH:  ${.CURDIR}/../../../../miscfs/genfs

LIB=	rumpvfs_layerfs
COMMENT=Layer file system support (used by other drivers)

SRCS=	layer_subr.c layer_vfsops.c layer_vnops.c

CWARNFLAGS.clang+=	-Wno-conversion

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
