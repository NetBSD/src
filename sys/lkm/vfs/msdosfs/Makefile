#	$NetBSD: Makefile,v 1.4 1998/03/10 06:53:36 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/msdosfs ${.CURDIR}/..

NOMAN=	yep

KMOD=		msdosfs

SRCS=	lkminit_vfs.c
SRCS+=	msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c
SRCS+=	msdosfs_vfsops.c msdosfs_vnops.c

.include <bsd.kmod.mk>

# XXX
.if defined(UVM)
CPPFLAGS+=      -DUVM
.endif
