#	$NetBSD: Makefile,v 1.24.2.1 2026/11/04 23:28:53 perseant Exp $

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
COPTS.pf.c+=	${CC_WNO_STRINGOP_TRUNCATION}

# XXX GCC 14
.if ${MACHINE} == "alpha" && ${HAVE_GCC:U0} == 14
COPTS.file.c+=	-O0
.endif

.include <bsd.lib.mk>
