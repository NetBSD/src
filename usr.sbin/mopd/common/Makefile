#	$NetBSD: Makefile,v 1.7 2003/10/19 06:12:07 lukem Exp $

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

LIB=	common
SRCS=	cmp.c device.c dl.c file.c get.c log.c loop-bsd.c mopdef.c nma.c pf.c \
	print.c put.c rc.c version.c
CLEANFILES= version.c

version.c: VERSION
	${_MKMSGCREATE}
	${_MKCMD}\
	rm -f version.c; \
	sed 's/.*/char version[] = "&";/' ${.ALLSRC} > version.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
