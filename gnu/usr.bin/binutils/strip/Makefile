#	$NetBSD: Makefile,v 1.4 1999/01/07 05:56:30 itohy Exp $

.if (${MACHINE_ARCH} != "powerpc")
PROG=	strip
SRCS=	objcopy.c is-strip.c
.endif

MAN=	strip.1

.include <bsd.prog.mk>

# STRIPPROG is capable of stripping itself, so this is OK.
.if (${STRIPFLAG} != "") && (${MACHINE_ARCH} != "powerpc")
STRIPFLAG:=
realinstall: __installstrip
__installstrip:
	chmod 755 ${DESTDIR}${BINDIR}/strip
	${STRIPPROG} ${DESTDIR}${BINDIR}/strip
	chmod ${BINMODE} ${DESTDIR}${BINDIR}/strip
.endif
