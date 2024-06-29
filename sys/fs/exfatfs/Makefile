#	$NetBSD: Makefile,v 1.1.2.1 2024/06/29 19:43:26 perseant Exp $

.include <bsd.own.mk>

CFLAGS=-I../.. -I.

.if ${MKEXFATFS} != "no"
INCSDIR= /usr/include/fs/exfatfs
INCS=	exfatfs.h exfatfs_cksum.h exfatfs_conv.h exfatfs_dirent.h exfatfs_extern.h exfatfs_inode.h exfatfs_mount.h exfatfs_tables.h exfatfs_trie.h exfatfs_vfsops.h exfatfs_vnops.h
.endif

.include <bsd.kinc.mk>

USEROBJ=exfatfs_cksum.o exfatfs_conv.o exfatfs_tables.o exfatfs_trie.o exfatfs_vfsops.o exfatfs_vnops.o
userland: $(USEROBJ)

userclean:
	rm -f $(USEROBJ)

