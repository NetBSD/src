#	$NetBSD: Makefile,v 1.2 2003/09/06 08:40:42 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/coda ${.CURDIR}/../coda

KMOD=	coda5

SRCS=	lkminit_vfs.c
SRCS+=	coda_namecache.c coda_psdev.c coda_subr.c coda_venus.c
SRCS+=	coda_vfsops.c coda_vnops.c

CPPFLAGS+=	-DCODA_COMPAT_5

.include <bsd.kmod.mk>
