#	from: @{#}Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.9.8.1 2000/08/07 01:52:02 mrg Exp $

.include <bsd.own.mk>

.if (${MACHINE_ARCH} != "sparc64")
PROG=		g77
SRCS=		gcc.c g77spec.c g77version.o version.c choose-temp.c \
		pexecute.c prefix.c mkstemp.c obstack.c
BINDIR=		/usr/bin
CPPFLAGS+=	-DLANG_SPECIFIC_DRIVER

LINKS+=		${BINDIR}/g77 ${BINDIR}/f77 ${BINDIR}/g77 ${BINDIR}/fort77
.endif

MAN=		g77.1
MLINKS=		g77.1 f77.1 g77.1 fort77.1

.include <bsd.prog.mk>

.if (${MACHINE_ARCH} != "sparc64")
g77version.o: ${DIST}/f/version.c
	${COMPILE.c} -o $@ $>

CPPFLAGS+=	-I${DIST}/f
.endif

.PATH:		${DIST}/f
