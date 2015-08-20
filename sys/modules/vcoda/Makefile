#	$NetBSD: Makefile,v 1.2 2015/08/20 11:05:02 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda

KMOD=	vcoda
IOCONF=	vcoda.ioconf
SRCS=	coda_psdev.c coda_namecache.c coda_subr.c

.include <bsd.kmodule.mk>
