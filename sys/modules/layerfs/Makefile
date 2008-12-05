#	$NetBSD: Makefile,v 1.1 2008/12/05 13:05:37 ad Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/genfs

KMOD=	layerfs
SRCS=	layer_subr.c layer_vfsops.c layer_vnops.c

.include <bsd.kmodule.mk>
