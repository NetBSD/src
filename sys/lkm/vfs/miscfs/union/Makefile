#	$NetBSD: Makefile,v 1.5 1998/02/18 03:55:36 perry Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/union ${.CURDIR}/../..

NOMAN=	yep

KMOD=		union

SRCS=	lkminit_vfs.c
SRCS+=	union_vfsops.c union_vnops.c union_subr.c

.include <bsd.kmod.mk>

# XXX
.if defined(UVM)
CPPFLAGS+=      -DUVM
.endif
.if defined(PMAP_NEW)
CPPFLAGS+=      -DPMAP_NEW
.endif
