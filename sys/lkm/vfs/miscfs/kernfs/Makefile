#	$NetBSD: Makefile,v 1.5 1998/02/18 03:55:23 perry Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/kernfs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		kernfs

SRCS=	lkminit_vfs.c
SRCS+=	kernfs_vfsops.c kernfs_vnops.c

.include <bsd.kmod.mk>

# XXX
.if defined(UVM)
CPPFLAGS+=      -DUVM
.endif
.if defined(PMAP_NEW)
CPPFLAGS+=      -DPMAP_NEW
.endif
