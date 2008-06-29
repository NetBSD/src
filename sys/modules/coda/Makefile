#	$NetBSD: Makefile,v 1.1.2.2 2008/06/29 09:33:16 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda

KMOD=	coda

SRCS=	coda_namecache.c coda_psdev.c coda_subr.c coda_venus.c \
	coda_vfsops.c coda_vnops.c

.include <bsd.kmodule.mk>
