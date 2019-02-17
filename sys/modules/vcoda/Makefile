#	$NetBSD: Makefile,v 1.3 2019/02/17 04:06:00 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda

KMOD=	vcoda
IOCONF=	vcoda.ioconf
SRCS=	coda_psdev.c coda_namecache.c coda_subr.c

WARNS=	3

.include <bsd.kmodule.mk>
