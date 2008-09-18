#	$NetBSD: Makefile,v 1.11.2.1 2008/09/18 04:30:07 wrstuden Exp $

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

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4 || defined(HAVE_PCC)
COPTS.print.c+=	-Wno-pointer-sign
.endif
