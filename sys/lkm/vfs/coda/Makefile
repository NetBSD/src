#	$NetBSD: Makefile,v 1.3 1999/02/13 02:54:52 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/coda ${.CURDIR}/..

MKMAN=	no

KMOD=	coda

SRCS=	lkminit_vfs.c
SRCS+=	coda_namecache.c coda_psdev.c coda_subr.c coda_venus.c
SRCS+=	coda_vfsops.c coda_vnops.c

.include <bsd.kmod.mk>
