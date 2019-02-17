#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:50 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/hfs

KMOD=  	hfs
SRCS=	hfs_subr.c hfs_vfsops.c hfs_vnops.c libhfs.c unicode.c

WARNS=	3

.include <bsd.kmodule.mk>
