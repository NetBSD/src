#	$NetBSD: Makefile,v 1.2.2.1 2017/12/03 11:38:50 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda

KMOD=	coda
IOCONF=	coda.ioconf
SRCS=	coda_venus.c coda_vfsops.c coda_vnops.c

.include <bsd.kmodule.mk>
