#	$NetBSD: Makefile,v 1.1.8.2 2007/09/03 14:45:06 yamt Exp $
#

.include <bsd.own.mk>

LIB=	hfs

.PATH:  ${NETBSDSRCDIR}/sys/fs/hfs

SRCS=	hfs_nhash.c hfs_subr.c hfs_vfsops.c hfs_vnops.c libhfs.c unicode.c

CFLAGS+=-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
