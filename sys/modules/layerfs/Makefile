#	$NetBSD: Makefile,v 1.1.4.2 2009/01/17 13:29:30 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/genfs

KMOD=	layerfs
SRCS=	layer_subr.c layer_vfsops.c layer_vnops.c

.include <bsd.kmodule.mk>
