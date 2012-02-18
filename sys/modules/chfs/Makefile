#	$NetBSD: Makefile,v 1.1.2.1 2012/02/18 07:35:36 mrg Exp $

.include "../Makefile.inc"

.PATH:	${S}/ufs/chfs

CPPFLAGS+=	-DDIAGNOSTIC -DDEBUG -DLOCKDEBUG

KMOD=	chfs
SRCS=	ebh.c debug.c chfs_wbuf.c chfs_vnode_cache.c chfs_ihash.c
SRCS+=  chfs_gc.c
SRCS+=  chfs_vnode.c chfs_erase.c chfs_write.c chfs_readinode.c
SRCS+=  chfs_build.c chfs_scan.c chfs_nodeops.c chfs_malloc.c
SRCS+=	chfs_pool.c chfs_subr.c chfs_vfsops.c chfs_vnops.c

.include <bsd.kmodule.mk>
