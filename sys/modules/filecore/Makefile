#	$NetBSD: Makefile,v 1.1.74.1 2019/06/10 22:09:25 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/filecorefs

KMOD=	filecore
SRCS=	filecore_bmap.c filecore_lookup.c filecore_node.c filecore_utils.c \
	filecore_vfsops.c filecore_vnops.c

WARNS=	3

.include <bsd.kmodule.mk>
