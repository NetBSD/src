#	$NetBSD: Makefile,v 1.9 2004/05/23 02:24:52 lukem Exp $

MKPRIVATELIB=	yes

LIB=	common
SRCS=	cmp.c device.c dl.c file.c get.c log.c loop-bsd.c mopdef.c nma.c pf.c \
	print.c put.c rc.c version.c
CLEANFILES= version.c

version.c: VERSION
	${_MKTARGET_CREATE}
	rm -f version.c; \
	sed 's/.*/char version[] = "&";/' ${.ALLSRC} > version.c

.include <bsd.lib.mk>
