#	$NetBSD: Makefile,v 1.3 1999/03/13 23:22:28 lukem Exp $

LIB=	common
SRCS=	cmp.c device.c dl.c file.c get.c loop-bsd.c mopdef.c nma.c pf.c \
	print.c put.c rc.c version.c
CLEANFILES= version.c

version.c: VERSION
	rm -f version.c; \
	sed 's/.*/char version[] = "&";/' ${.ALLSRC} > version.c

MKLINT=		no
MKPIC=		no
MKPROFILE=	no

# only needed during build
libinstall::

.include <bsd.lib.mk>
