#	$NetBSD: Makefile,v 1.4 2001/12/12 01:50:15 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/coda ${.CURDIR}/..

NOMAN=	# defined

KMOD=	coda

SRCS=	lkminit_vfs.c
SRCS+=	coda_namecache.c coda_psdev.c coda_subr.c coda_venus.c
SRCS+=	coda_vfsops.c coda_vnops.c

.include <bsd.kmod.mk>
