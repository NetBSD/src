#	$NetBSD: Makefile,v 1.8 2003/10/21 10:01:22 lukem Exp $

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

LIB=	common
SRCS=	cmp.c device.c dl.c file.c get.c log.c loop-bsd.c mopdef.c nma.c pf.c \
	print.c put.c rc.c version.c
CLEANFILES= version.c

version.c: VERSION
	${_MKTARGET_CREATE}
	rm -f version.c; \
	sed 's/.*/char version[] = "&";/' ${.ALLSRC} > version.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
