#	$NetBSD: Makefile,v 1.2 1999/02/13 02:55:00 lukem Exp $

LIB=	common
SRCS=	cmp.c device.c dl.c file.c get.c loop-bsd.c mopdef.c nma.c pf.c \
	print.c put.c rc.c version.c
CLEANFILES= version.c

version.c: VERSION
	rm -f version.c; \
	sed 's/.*/char version[] = "&";/' ${.ALLSRC} > version.c

MKPROFILE=	no
MKPIC=		no

# only needed during build
libinstall::

.include <bsd.lib.mk>
