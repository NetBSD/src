#	$NetBSD: Makefile,v 1.4 1998/02/11 11:57:21 mrg Exp $

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
