#	$NetBSD: Makefile,v 1.6 2002/12/23 17:23:13 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/adosfs ${.CURDIR}/..

KMOD=	adosfs

SRCS=	lkminit_vfs.c
SRCS+=	adlookup.c adutil.c advfsops.c advnops.c

.include <bsd.kmod.mk>
