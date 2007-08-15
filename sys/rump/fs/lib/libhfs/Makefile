#	$NetBSD: Makefile,v 1.1.2.2 2007/08/15 13:50:23 skrll Exp $
#

.include <bsd.own.mk>

LIB=	hfs

.PATH:  ${NETBSDSRCDIR}/sys/fs/hfs

SRCS=	hfs_nhash.c hfs_subr.c hfs_vfsops.c hfs_vnops.c libhfs.c unicode.c

CFLAGS+=-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
