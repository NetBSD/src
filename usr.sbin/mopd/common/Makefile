#	$NetBSD: Makefile,v 1.5 2002/09/18 13:31:54 lukem Exp $

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

LIB=	common
SRCS=	cmp.c device.c dl.c file.c get.c loop-bsd.c mopdef.c nma.c pf.c \
	print.c put.c rc.c version.c
CLEANFILES= version.c

version.c: VERSION
	rm -f version.c; \
	sed 's/.*/char version[] = "&";/' ${.ALLSRC} > version.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
