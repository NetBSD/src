#	$NetBSD: Makefile,v 1.5 1999/02/02 22:06:02 tv Exp $

PROG=	strip
SRCS=	objcopy.c is-strip.c
MAN=	strip.1

.include <bsd.prog.mk>

# STRIPPROG is capable of stripping itself, so this is OK.
.if (${STRIPFLAG} != "")
STRIPFLAG:=
realinstall: __installstrip
__installstrip:
	chmod 755 ${DESTDIR}${BINDIR}/strip
	${STRIPPROG} ${DESTDIR}${BINDIR}/strip
	chmod ${BINMODE} ${DESTDIR}${BINDIR}/strip
.endif
