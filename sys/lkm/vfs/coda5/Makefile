#	$NetBSD: Makefile,v 1.1 2003/09/02 12:08:45 drochner Exp $

.include "../Makefile.inc"

.PATH:	$S/coda ${.CURDIR}/..

KMOD=	coda5

SRCS=	lkminit_vfs.c
SRCS+=	coda_namecache.c coda_psdev.c coda_subr.c coda_venus.c
SRCS+=	coda_vfsops.c coda_vnops.c

CPPFLAGS+=	-DCODA_COMPAT_5

.include <bsd.kmod.mk>
