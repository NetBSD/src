#	$NetBSD: Makefile,v 1.7 1998/03/28 23:58:45 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/procfs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		procfs

SRCS=	lkminit_vfs.c
SRCS+=	procfs_ctl.c procfs_note.c procfs_status.c procfs_subr.c
SRCS+=  procfs_vfsops.c procfs_vnops.c

.include <bsd.kmod.mk>
