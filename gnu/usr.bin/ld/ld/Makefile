#	$NetBSD: Makefile,v 1.3 1997/06/23 13:25:54 mrg Exp $

.if	(${MACHINE_ARCH} == "i386") || \
	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "vax") || \
	(${MACHINE_ARCH} == "arm32")
PROG=	ld
SRCS=	ld.c symbol.c lib.c shlib.c warnings.c etc.c rrs.c xbits.c md.c
LDADD+= -lgnumalloc
DPADD+= /usr/lib/libgnumalloc.a
BINDIR=/usr/bin
.endif

MAN=	ld.1

.include <bsd.prog.mk>
