#	$NetBSD: Makefile,v 1.5 2001/12/12 12:06:47 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/adosfs ${.CURDIR}/..

KMOD=	adosfs

SRCS=	lkminit_vfs.c
SRCS+=	adlookup.c adutil.c advfsops.c advnops.c

.include <bsd.kmod.mk>
