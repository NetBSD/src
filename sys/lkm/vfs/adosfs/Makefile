#	$NetBSD: Makefile,v 1.3.20.2 2002/12/29 20:55:01 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/adosfs ${.CURDIR}/..

KMOD=	adosfs

SRCS=	lkminit_vfs.c
SRCS+=	adlookup.c adutil.c advfsops.c advnops.c

.include <bsd.kmod.mk>
