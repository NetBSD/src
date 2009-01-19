#	$NetBSD: Makefile,v 1.1.6.2 2009/01/19 13:20:09 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/genfs

KMOD=	layerfs
SRCS=	layer_subr.c layer_vfsops.c layer_vnops.c

.include <bsd.kmodule.mk>
