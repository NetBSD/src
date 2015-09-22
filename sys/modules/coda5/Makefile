#	$NetBSD: Makefile,v 1.1.68.1 2015/09/22 12:06:09 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda 

KMOD=	coda5
IOCONF=	coda5.ioconf
SRCS=	coda_namecache.c coda_psdev.c coda_subr.c coda_venus.c \
	coda_vfsops.c coda_vnops.c

CPPFLAGS+=	-DCODA_COMPAT_5

.include <bsd.kmodule.mk>
