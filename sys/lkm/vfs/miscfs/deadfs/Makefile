#	$NetBSD: Makefile,v 1.1 1996/08/22 20:18:29 explorer Exp $

.PATH:	/sys/miscfs/deadfs

NOMAN=	yep

GENERATE=	vnode_if.h
CLEANFILES+=	vnode_if.[ch]

LKM=	deadfs

SRCS=	lkminit_deadfs.c
SRCS+=	dead_vnops.c

vnode_if.h: /sys/kern/vnode_if.sh /sys/kern/vnode_if.src
	sh /sys/kern/vnode_if.sh /sys/kern/vnode_if.src

.include <bsd.lkm.mk>
