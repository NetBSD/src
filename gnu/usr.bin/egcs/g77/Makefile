#	from: @{#}Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.4 1998/04/03 02:20:30 tv Exp $

PROG=	g77
SRCS=	gcc.c g77spec.c version.c choose-temp.c pexecute.c obstack.c
BINDIR=	/usr/bin

LINKS+=	${BINDIR}/g77 ${BINDIR}/f77

MAN=	g77.1
MLINKS=	g77.1 f77.1

.include <bsd.prog.mk>

.PATH: ${DIST}/f ${DIST}
CPPFLAGS+= -I${DIST}/f -I${DIST} -DLANG_SPECIFIC_DRIVER
