#	$NetBSD: Makefile,v 1.1.2.2 2008/06/29 09:33:16 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/filecorefs

KMOD=	filecorefs
SRCS=	filecore_bmap.c filecore_lookup.c filecore_node.c filecore_utils.c \
	filecore_vfsops.c filecore_vnops.c

.include <bsd.kmodule.mk>
