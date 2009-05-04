#	$NetBSD: Makefile,v 1.1.20.2 2009/05/04 08:14:06 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda

KMOD=	coda

SRCS=	coda_namecache.c coda_psdev.c coda_subr.c coda_venus.c \
	coda_vfsops.c coda_vnops.c

.include <bsd.kmodule.mk>
