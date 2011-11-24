#	$NetBSD: Makefile,v 1.1 2011/11/24 15:52:24 ahoka Exp $

.include "../Makefile.inc"

.PATH:	${S}/ufs/chfs

CPPFLAGS+=	-DDIAGNOSTIC -DDEBUG -DLOCKDEBUG
CFLAGS+=	-g

KMOD=	chfs
SRCS=	ebh.c debug.c chfs_wbuf.c chfs_vnode_cache.c chfs_ihash.c
SRCS+=  chfs_gc.c
SRCS+=  chfs_vnode.c chfs_erase.c chfs_write.c chfs_readinode.c
SRCS+=  chfs_build.c chfs_scan.c chfs_nodeops.c chfs_malloc.c
SRCS+=	chfs_pool.c chfs_subr.c chfs_vfsops.c chfs_vnops.c

.include <bsd.kmodule.mk>
