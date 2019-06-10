#	$NetBSD: Makefile,v 1.8.18.1 2019/06/10 22:09:31 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/ufs/lfs

lfs_vnops.o: ulfs_readwrite.c

KMOD=	lfs
CPPFLAGS+=#-DLFS_DIRHASH -DLFS_EI -DLFS_QUOTA -DLFS_QUOTA2

SRCS=	lfs_vfsops.c lfs_vnops.c lfs_subr.c lfs_alloc.c lfs_balloc.c \
	lfs_bio.c lfs_cksum.c lfs_debug.c lfs_inode.c lfs_pages.c \
	lfs_segment.c lfs_rename.c lfs_syscalls.c lfs_itimes.c

SRCS+=	ulfs_bmap.c ulfs_dirhash.c ulfs_inode.c ulfs_lookup.c \
	ulfs_snapshot.c ulfs_vfsops.c ulfs_vnops.c

WARNS=	3

.if ${MACHINE} == "vax"
# GCC 6.5 and 7.4 cannot compile this with -DDIAGNOSTIC and -O2/-O1
COPTS.lfs_inode.c+=	-O0
.endif

.include <bsd.kmodule.mk>
