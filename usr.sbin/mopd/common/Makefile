#	$NetBSD: Makefile,v 1.10 2006/05/12 01:54:57 mrg Exp $

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

.if ${HAVE_GCC} == 4
COPTS.print.c+=	-Wno-pointer-sign
.endif
