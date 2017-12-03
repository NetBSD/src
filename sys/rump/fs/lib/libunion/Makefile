#	$NetBSD: Makefile,v 1.1.24.1 2017/12/03 11:39:13 jdolecek Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/union

LIB=	rumpfs_union
COMMENT=union file system (fan-out layer)

SRCS=	union_subr.c union_vfsops.c union_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
