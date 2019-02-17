#	$NetBSD: Makefile,v 1.4 2019/02/17 04:05:44 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda

KMOD=	coda
IOCONF=	coda.ioconf
SRCS=	coda_venus.c coda_vfsops.c coda_vnops.c

WARNS=	3

.include <bsd.kmodule.mk>
