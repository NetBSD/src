#	$NetBSD: Makefile,v 1.4 2001/12/12 01:50:15 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/adosfs ${.CURDIR}/..

NOMAN=	# defined

KMOD=	adosfs

SRCS=	lkminit_vfs.c
SRCS+=	adlookup.c adutil.c advfsops.c advnops.c

.include <bsd.kmod.mk>
