#	$NetBSD: Makefile,v 1.1 1996/08/22 20:18:35 explorer Exp $

.PATH:	/sys/miscfs/fifofs

NOMAN=	yep

GENERATE=	vnode_if.h
CLEANFILES+=	vnode_if.[ch]

#
# fifo needs this.  Can this module even be loaded?
CFLAGS+=	-DFIFO

LKM=	fifofs

SRCS=	lkminit_${LKM}.c
SRCS+=	fifo_vnops.c

vnode_if.h: /sys/kern/vnode_if.sh /sys/kern/vnode_if.src
	sh /sys/kern/vnode_if.sh /sys/kern/vnode_if.src

.include <bsd.lkm.mk>
