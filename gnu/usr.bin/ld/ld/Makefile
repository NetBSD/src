#	$NetBSD: Makefile,v 1.5 1998/12/17 14:34:51 pk Exp $

.if	(${MACHINE_ARCH} == "i386") || \
	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "vax") || \
	(${MACHINE_ARCH} == "arm32")
PROG=	ld
SRCS=	ld.c symbol.c lib.c shlib.c warnings.c rrs.c xbits.c md.c
.if defined(DEBUG)
CPPFLAGS+=-DDEBUG
.endif
LDADD+= -lgnumalloc
DPADD+= /usr/lib/libgnumalloc.a
BINDIR=/usr/bin
.endif

MAN=	ld.1

.include <bsd.prog.mk>
