#	$NetBSD: Makefile,v 1.13 2008/10/25 22:27:39 apb Exp $

LIBISPRIVATE=	yes

LIB=	common
SRCS=	cmp.c device.c dl.c file.c get.c log.c loop-bsd.c mopdef.c nma.c pf.c \
	print.c put.c rc.c version.c
CLEANFILES= version.c

version.c: VERSION
	${_MKTARGET_CREATE}
	rm -f version.c; \
	${TOOL_SED} 's/.*/char version[] = "&";/' ${.ALLSRC} > version.c

.include <bsd.lib.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} == 4 || defined(HAVE_PCC)
COPTS.print.c+=	-Wno-pointer-sign
.endif
