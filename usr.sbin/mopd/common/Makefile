#	$NetBSD: Makefile,v 1.23 2021/10/31 18:06:42 christos Exp $

LIBISPRIVATE=	yes

.include <bsd.init.mk>

LIB=	common
SRCS=	cmp.c device.c dl.c file.c get.c log.c loop-bsd.c mopdef.c nma.c pf.c \
	print.c put.c rc.c version.c
CLEANFILES= version.c

version.c: VERSION
	${_MKTARGET_CREATE}
	rm -f version.c; \
	${TOOL_SED} 's/.*/char version[] = "&";/' ${.ALLSRC} > version.c


COPTS.print.c+=	-Wno-pointer-sign
COPTS.pf.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.lib.mk>
