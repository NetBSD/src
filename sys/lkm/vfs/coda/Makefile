#	$NetBSD: Makefile,v 1.3.20.1 2002/01/08 00:33:21 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/coda ${.CURDIR}/..

KMOD=	coda

SRCS=	lkminit_vfs.c
SRCS+=	coda_namecache.c coda_psdev.c coda_subr.c coda_venus.c
SRCS+=	coda_vfsops.c coda_vnops.c

.include <bsd.kmod.mk>
