#	from: @{#}Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.5 1998/08/16 23:32:52 tv Exp $

PROG=		g77
SRCS=		gcc.c g77spec.c g77version.o version.c choose-temp.c \
		pexecute.c prefix.o mkstemp.c obstack.c
BINDIR=		/usr/bin
CPPFLAGS+=	-DLANG_SPECIFIC_DRIVER

LINKS+=		${BINDIR}/g77 ${BINDIR}/f77

MAN=		g77.1
MLINKS=		g77.1 f77.1

.include <bsd.prog.mk>

g77version.o: ${DIST}/f/version.c
	${COMPILE.c} -o $@ $>

.PATH:		${DIST}/f
CPPFLAGS+=	-I${DIST}/f
