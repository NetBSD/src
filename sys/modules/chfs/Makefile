#	$NetBSD: Makefile,v 1.4.18.1 2019/06/10 22:09:12 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/ufs/chfs

CPPFLAGS+=	-DDIAGNOSTIC -DDEBUG -DLOCKDEBUG

KMOD=	chfs
SRCS=	ebh.c chfs_wbuf.c chfs_vnode_cache.c
SRCS+=  chfs_gc.c
SRCS+=  chfs_vnode.c chfs_erase.c chfs_write.c chfs_readinode.c
SRCS+=  chfs_build.c chfs_scan.c chfs_nodeops.c chfs_malloc.c
SRCS+=	chfs_pool.c chfs_subr.c chfs_vfsops.c chfs_vnops.c

WARNS=	3

.include <bsd.kmodule.mk>
