#	$NetBSD: Makefile,v 1.5 1998/02/11 11:57:23 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/procfs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		procfs

SRCS=	lkminit_vfs.c
SRCS+=	procfs_ctl.c procfs_note.c procfs_status.c procfs_subr.c
SRCS+=  procfs_vfsops.c procfs_vnops.c

.include <bsd.kmod.mk>

# XXX
.if defined(UVM)
CPPFLAGS+=      -DUVM
.endif
