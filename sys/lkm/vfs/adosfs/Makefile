#	$NetBSD: Makefile,v 1.1 1997/07/08 16:44:49 veego Exp $

.include "../Makefile.inc"

.PATH:	$S/adosfs ${.CURDIR}/..

NOMAN=	yep

KMOD=		adosfs

SRCS=	lkminit_vfs.c
SRCS+=	adlookup.c adutil.c advfsops.c advnops.c

.include <bsd.kmod.mk>
