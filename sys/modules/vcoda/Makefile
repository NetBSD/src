#	$NetBSD: Makefile,v 1.1.18.1 2015/09/22 12:06:10 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda

KMOD=	vcoda
IOCONF=	vcoda.ioconf
SRCS=	coda_psdev.c coda_namecache.c coda_subr.c

.include <bsd.kmodule.mk>
