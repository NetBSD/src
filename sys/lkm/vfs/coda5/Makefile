#	$NetBSD: Makefile,v 1.3 2003/09/07 12:25:54 tron Exp $

.include "../Makefile.inc"

.PATH:	$S/coda ${.CURDIR}/../coda5

KMOD=	coda5

LNFILES=	${.CURDIR}/../coda/lkminit_vfs.c lkminit_vfs.c

SRCS=	lkminit_vfs.c
SRCS+=	coda_namecache.c coda_psdev.c coda_subr.c coda_venus.c
SRCS+=	coda_vfsops.c coda_vnops.c

CPPFLAGS+=	-DCODA_COMPAT_5

.include <bsd.kmod.mk>
