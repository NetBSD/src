#	from: @{#}Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.10 2000/08/06 15:26:56 mrg Exp $

.include <bsd.own.mk>

.if (${MACHINE_ARCH} != "sparc64")
PROG=		g77
SRCS=		gcc.c g77spec.c g77version.o version.c choose-temp.c \
		pexecute.c prefix.c mkstemp.c obstack.c
BINDIR=		/usr/bin
CPPFLAGS+=	-DLANG_SPECIFIC_DRIVER
CPPFLAGS+=	-I${DIST}/f

LINKS+=		${BINDIR}/g77 ${BINDIR}/f77 ${BINDIR}/g77 ${BINDIR}/fort77

g77version.o: ${DIST}/f/version.c
	${COMPILE.c} -o $@ $>
.endif

MAN=		g77.1
MLINKS=		g77.1 f77.1 g77.1 fort77.1

.include <bsd.prog.mk>

.PATH:		${DIST}/f
