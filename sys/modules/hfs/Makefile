#	$NetBSD: Makefile,v 1.2.28.1 2019/06/10 22:09:26 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/hfs

KMOD=  	hfs
SRCS=	hfs_subr.c hfs_vfsops.c hfs_vnops.c libhfs.c unicode.c

WARNS=	3

.include <bsd.kmodule.mk>
