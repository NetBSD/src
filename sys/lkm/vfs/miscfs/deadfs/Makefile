#	$NetBSD: Makefile,v 1.3 1996/08/27 21:44:37 explorer Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/deadfs

NOMAN=	yep

LKM=	deadfs

SRCS=	lkminit_deadfs.c
SRCS+=	dead_vnops.c

.include <bsd.kmod.mk>
