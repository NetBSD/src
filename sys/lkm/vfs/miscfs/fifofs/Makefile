#	$NetBSD: Makefile,v 1.2 1996/08/23 05:39:14 mrg Exp $

.include "../Makefile.inc"

.PATH:	$s/miscfs/fifofs

NOMAN=	yep

GENERATE=	vnode_if.h
CLEANFILES+=	vnode_if.[ch]

#
# fifo needs this.  Can this module even be loaded?
CFLAGS+=	-DFIFO

LKM=	fifofs

SRCS=	lkminit_${LKM}.c
SRCS+=	fifo_vnops.c

vnode_if.h: $S/kern/vnode_if.sh $S/kern/vnode_if.src
	sh $S/kern/vnode_if.sh $S/kern/vnode_if.src

.include <bsd.lkm.mk>
