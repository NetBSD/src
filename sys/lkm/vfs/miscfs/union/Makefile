#	$NetBSD: Makefile,v 1.4 1998/02/11 11:57:22 mrg Exp $

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
