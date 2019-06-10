#	$NetBSD: Makefile,v 1.3.18.1 2019/06/10 22:09:13 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda

KMOD=	coda
IOCONF=	coda.ioconf
SRCS=	coda_venus.c coda_vfsops.c coda_vnops.c

WARNS=	3

.include <bsd.kmodule.mk>
