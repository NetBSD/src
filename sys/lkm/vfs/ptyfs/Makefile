#	$NetBSD: Makefile,v 1.2 2005/12/11 12:24:50 christos Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/ptyfs ${.CURDIR}/../.. $S/arch/${MACHINE}/${MACHINE} \
	$S/arch/${MACHINE_CPU}/${MACHINE_CPU}

KMOD=	ptyfs

SRCS=	lkminit_vfs.c
SRCS+=	ptyfs_vfsops.c ptyfs_vnops.c ptyfs_subr.c

.include <bsd.kmod.mk>
