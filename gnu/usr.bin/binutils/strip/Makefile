#	$NetBSD: Makefile,v 1.3.2.3 1998/11/09 08:09:01 cgd Exp $

# XXX until our binutils is upgraded 
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
	${STRIPPROG} ${DESTDIR}/${BINDIR}/strip
.endif
