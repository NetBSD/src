#	$NetBSD: Makefile,v 1.1.2.2 2004/11/14 08:16:13 skrll Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/ptyfs ${.CURDIR}/../.. $S/arch/${MACHINE}/${MACHINE} \
	$S/arch/${MACHINE_CPU}/${MACHINE_CPU}

KMOD=	ptyfs

SRCS=	lkminit_vfs.c
SRCS+=	ptyfs_vfsops.c ptyfs_vnops.c ptyfs_subr.c

.include <bsd.kmod.mk>
