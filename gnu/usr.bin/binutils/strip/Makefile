#	$NetBSD: Makefile,v 1.8 2001/08/06 19:12:06 tv Exp $

PROG=		strip

.include <bsd.own.mk>
.if defined(USE_NEW_TOOLCHAIN)
.include "${.CURDIR}/../common/Makefile.prog"
.else
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
.endif
