#	$NetBSD: Makefile,v 1.1.42.1 2015/12/27 12:10:15 skrll Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/union

LIB=	rumpfs_union
COMMENT=union file system (fan-out layer)

SRCS=	union_subr.c union_vfsops.c union_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
