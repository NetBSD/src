#	$NetBSD: Makefile,v 1.1.48.1 2014/08/20 00:04:33 tls Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/hfs

KMOD=  	hfs
SRCS=	hfs_subr.c hfs_vfsops.c hfs_vnops.c libhfs.c unicode.c

.include <bsd.kmodule.mk>
