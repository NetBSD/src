#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:49 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/filecorefs

KMOD=	filecore
SRCS=	filecore_bmap.c filecore_lookup.c filecore_node.c filecore_utils.c \
	filecore_vfsops.c filecore_vnops.c

WARNS=	3

.include <bsd.kmodule.mk>
