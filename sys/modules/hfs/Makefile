#	$NetBSD: Makefile,v 1.2 2014/08/10 08:53:22 hannken Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/hfs

KMOD=  	hfs
SRCS=	hfs_subr.c hfs_vfsops.c hfs_vnops.c libhfs.c unicode.c

.include <bsd.kmodule.mk>
