#	$NetBSD: Makefile,v 1.3.20.1 2002/01/08 00:33:21 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/adosfs ${.CURDIR}/..

KMOD=	adosfs

SRCS=	lkminit_vfs.c
SRCS+=	adlookup.c adutil.c advfsops.c advnops.c

.include <bsd.kmod.mk>
