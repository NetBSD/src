#	$NetBSD: Makefile,v 1.3 2015/08/20 11:05:00 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda

KMOD=	coda
IOCONF=	coda.ioconf
SRCS=	coda_venus.c coda_vfsops.c coda_vnops.c

.include <bsd.kmodule.mk>
