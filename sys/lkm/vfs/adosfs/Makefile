#	$NetBSD: Makefile,v 1.2 1999/02/12 05:41:58 cjs Exp $

.include "../Makefile.inc"

.PATH:	$S/adosfs ${.CURDIR}/..

MKMAN=	no

KMOD=		adosfs

SRCS=	lkminit_vfs.c
SRCS+=	adlookup.c adutil.c advfsops.c advnops.c

.include <bsd.kmod.mk>
