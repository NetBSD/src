#	$NetBSD: Makefile,v 1.1.4.2 2008/07/03 18:38:19 simonb Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/filecorefs

KMOD=	filecorefs
SRCS=	filecore_bmap.c filecore_lookup.c filecore_node.c filecore_utils.c \
	filecore_vfsops.c filecore_vnops.c

.include <bsd.kmodule.mk>
