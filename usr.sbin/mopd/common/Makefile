#	$NetBSD: Makefile,v 1.10.18.1 2008/05/18 12:36:22 yamt Exp $

LIBISPRIVATE=	yes

LIB=	common
SRCS=	cmp.c device.c dl.c file.c get.c log.c loop-bsd.c mopdef.c nma.c pf.c \
	print.c put.c rc.c version.c
CLEANFILES= version.c

version.c: VERSION
	${_MKTARGET_CREATE}
	rm -f version.c; \
	sed 's/.*/char version[] = "&";/' ${.ALLSRC} > version.c

.include <bsd.lib.mk>

.if ${HAVE_GCC} == 4
COPTS.print.c+=	-Wno-pointer-sign
.endif
