#	$NetBSD: Makefile,v 1.1.4.2 2008/07/03 18:38:19 simonb Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda

KMOD=	coda

SRCS=	coda_namecache.c coda_psdev.c coda_subr.c coda_venus.c \
	coda_vfsops.c coda_vnops.c

.include <bsd.kmodule.mk>
