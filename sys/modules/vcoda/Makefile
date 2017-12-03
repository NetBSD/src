#	$NetBSD: Makefile,v 1.1.4.1 2017/12/03 11:39:01 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda

KMOD=	vcoda
IOCONF=	vcoda.ioconf
SRCS=	coda_psdev.c coda_namecache.c coda_subr.c

.include <bsd.kmodule.mk>
