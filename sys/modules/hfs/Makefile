#	$NetBSD: Makefile,v 1.1.4.2 2008/07/03 18:38:19 simonb Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/hfs

KMOD=  	hfs
SRCS=	hfs_nhash.c hfs_subr.c hfs_vfsops.c hfs_vnops.c libhfs.c \
	unicode.c

.include <bsd.kmodule.mk>
