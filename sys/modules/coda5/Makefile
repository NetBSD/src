#	$NetBSD: Makefile,v 1.2.18.1 2019/06/10 22:09:13 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/coda 

KMOD=	coda5
IOCONF=	coda5.ioconf
SRCS=	coda_namecache.c coda_psdev.c coda_subr.c coda_venus.c \
	coda_vfsops.c coda_vnops.c

CPPFLAGS+=	-DCODA_COMPAT_5

WARNS=	3

.include <bsd.kmodule.mk>
