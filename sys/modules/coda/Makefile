#	$NetBSD: Makefile,v 1.1 2008/06/28 16:11:35 rumble Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda

KMOD=	coda

SRCS=	coda_namecache.c coda_psdev.c coda_subr.c coda_venus.c \
	coda_vfsops.c coda_vnops.c

.include <bsd.kmodule.mk>
