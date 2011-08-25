#	$NetBSD: Makefile,v 1.18 2011/08/25 16:47:20 joerg Exp $

LIBISPRIVATE=	yes

LIB=	common
SRCS=	cmp.c device.c dl.c file.c get.c log.c loop-bsd.c mopdef.c nma.c pf.c \
	print.c put.c rc.c version.c
CLEANFILES= version.c

.include <bsd.own.mk>

.if ${MACHINE_CPU} == "mips"
CFLAGS+= -DNOAOUT
.endif

version.c: VERSION
	${_MKTARGET_CREATE}
	rm -f version.c; \
	${TOOL_SED} 's/.*/char version[] = "&";/' ${.ALLSRC} > version.c

.include <bsd.lib.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.print.c+=	-Wno-pointer-sign
.endif

COPTS.log.c+=	-Wno-format-nonliteral
