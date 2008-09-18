#	$NetBSD: Makefile,v 1.1.8.2 2008/09/18 04:36:58 wrstuden Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/hfs

KMOD=  	hfs
SRCS=	hfs_nhash.c hfs_subr.c hfs_vfsops.c hfs_vnops.c libhfs.c \
	unicode.c

.include <bsd.kmodule.mk>
