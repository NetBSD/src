#	$NetBSD: Makefile,v 1.3 1996/08/27 21:44:49 explorer Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/procfs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		procfs

SRCS=	lkminit_vfs.c
SRCS+=	procfs_vfsops.c procfs_vnops.c procfs_ctl.c procfs_note.c
SRCS+=	procfs_status.c procfs_subr.c

.include <bsd.kmod.mk>
