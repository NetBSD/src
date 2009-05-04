#	$NetBSD: Makefile,v 1.1.20.2 2009/05/04 08:14:10 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/hfs

KMOD=  	hfs
SRCS=	hfs_nhash.c hfs_subr.c hfs_vfsops.c hfs_vnops.c libhfs.c \
	unicode.c

.include <bsd.kmodule.mk>
