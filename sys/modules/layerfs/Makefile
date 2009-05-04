#	$NetBSD: Makefile,v 1.1.10.2 2009/05/04 08:14:10 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/genfs

KMOD=	layerfs
SRCS=	layer_subr.c layer_vfsops.c layer_vnops.c

.include <bsd.kmodule.mk>
