#	$NetBSD: Makefile,v 1.1 2004/11/11 01:43:25 christos Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/ptyfs ${.CURDIR}/../.. $S/arch/${MACHINE}/${MACHINE} \
	$S/arch/${MACHINE_CPU}/${MACHINE_CPU}

KMOD=	ptyfs

SRCS=	lkminit_vfs.c
SRCS+=	ptyfs_vfsops.c ptyfs_vnops.c ptyfs_subr.c

.include <bsd.kmod.mk>
