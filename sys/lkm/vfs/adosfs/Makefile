#	$NetBSD: Makefile,v 1.3 1999/02/13 02:54:52 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/adosfs ${.CURDIR}/..

MKMAN=	no

KMOD=	adosfs

SRCS=	lkminit_vfs.c
SRCS+=	adlookup.c adutil.c advfsops.c advnops.c

.include <bsd.kmod.mk>
