#	$NetBSD: Makefile,v 1.1.1.1 1998/09/25 15:01:56 rvb Exp $

.include "../Makefile.inc"

.PATH:	$S/coda ${.CURDIR}/..

NOMAN=	yep

KMOD=		coda

SRCS=	lkminit_vfs.c
SRCS+=	coda_namecache.c coda_psdev.c coda_subr.c coda_venus.c
SRCS+=	coda_vfsops.c coda_vnops.c

.include <bsd.kmod.mk>
