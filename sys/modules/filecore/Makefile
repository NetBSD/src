#	$NetBSD: Makefile,v 1.1 2008/11/24 11:26:49 ad Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/filecorefs

KMOD=	filecore
SRCS=	filecore_bmap.c filecore_lookup.c filecore_node.c filecore_utils.c \
	filecore_vfsops.c filecore_vnops.c

.include <bsd.kmodule.mk>
