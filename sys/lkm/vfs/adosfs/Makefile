#	$NetBSD: Makefile,v 1.3.22.1 2002/01/10 20:01:17 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/adosfs ${.CURDIR}/..

KMOD=	adosfs

SRCS=	lkminit_vfs.c
SRCS+=	adlookup.c adutil.c advfsops.c advnops.c

.include <bsd.kmod.mk>
