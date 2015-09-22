#	$NetBSD: Makefile,v 1.2.16.1 2015/09/22 12:06:08 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda

KMOD=	coda
IOCONF=	coda.ioconf
SRCS=	coda_venus.c coda_vfsops.c coda_vnops.c

.include <bsd.kmodule.mk>
