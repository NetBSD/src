#	$NetBSD: Makefile,v 1.18.2.1 2012/10/30 19:00:39 yamt Exp $

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

COPTS.print.c+=	-Wno-pointer-sign
COPTS.log.c+=	-Wno-format-nonliteral
