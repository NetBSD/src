#	$NetBSD: Makefile,v 1.2 1996/08/23 05:39:11 mrg Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/deadfs

NOMAN=	yep

GENERATE=	vnode_if.h
CLEANFILES+=	vnode_if.[ch]

LKM=	deadfs

SRCS=	lkminit_deadfs.c
SRCS+=	dead_vnops.c

vnode_if.h: $S/kern/vnode_if.sh $S/kern/vnode_if.src
	sh $S/kern/vnode_if.sh $S/kern/vnode_if.src

.include <bsd.lkm.mk>
