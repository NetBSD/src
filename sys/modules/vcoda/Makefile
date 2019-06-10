#	$NetBSD: Makefile,v 1.2.18.1 2019/06/10 22:09:42 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda

KMOD=	vcoda
IOCONF=	vcoda.ioconf
SRCS=	coda_psdev.c coda_namecache.c coda_subr.c

WARNS=	3

.include <bsd.kmodule.mk>
