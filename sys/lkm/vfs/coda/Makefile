#	$NetBSD: Makefile,v 1.3.22.1 2002/01/10 20:01:18 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/coda ${.CURDIR}/..

KMOD=	coda

SRCS=	lkminit_vfs.c
SRCS+=	coda_namecache.c coda_psdev.c coda_subr.c coda_venus.c
SRCS+=	coda_vfsops.c coda_vnops.c

.include <bsd.kmod.mk>
