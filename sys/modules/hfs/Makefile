#	$NetBSD: Makefile,v 1.1 2008/06/28 16:11:36 rumble Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/hfs

KMOD=  	hfs
SRCS=	hfs_nhash.c hfs_subr.c hfs_vfsops.c hfs_vnops.c libhfs.c \
	unicode.c

.include <bsd.kmodule.mk>
