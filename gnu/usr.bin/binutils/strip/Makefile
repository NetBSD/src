#	$NetBSD: Makefile,v 1.7 2000/02/01 06:02:01 tsutsui Exp $

PROG=		strip
SRCS=		objcopy.c is-strip.c
MAN=		strip.1

.include <bsd.prog.mk>

# STRIP is capable of stripping itself, so this is OK.
.if (${STRIPFLAG} != "")
STRIPFLAG:=
realinstall: __installstrip
__installstrip:
	chmod 755 ${DESTDIR}${BINDIR}/strip
	${STRIP} ${DESTDIR}${BINDIR}/strip
	chmod ${BINMODE} ${DESTDIR}${BINDIR}/strip
.endif
