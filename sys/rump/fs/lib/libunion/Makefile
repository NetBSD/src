#	$NetBSD: Makefile,v 1.2 2015/10/19 16:16:37 pooka Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/union

LIB=	rumpfs_union
COMMENT=union file system (fan-out layer)

SRCS=	union_subr.c union_vfsops.c union_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
