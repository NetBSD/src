#	$NetBSD: Makefile,v 1.3.10.1 1997/08/23 07:14:10 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/procfs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		procfs

SRCS=	lkminit_vfs.c
SRCS+=	procfs_ctl.c procfs_note.c procfs_status.c procfs_subr.c
SRCS+=  procfs_vfsops.c procfs_vnops.c

.include <bsd.kmod.mk>
