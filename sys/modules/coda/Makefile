#	$NetBSD: Makefile,v 1.1.8.2 2008/09/18 04:36:58 wrstuden Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda

KMOD=	coda

SRCS=	coda_namecache.c coda_psdev.c coda_subr.c coda_venus.c \
	coda_vfsops.c coda_vnops.c

.include <bsd.kmodule.mk>
