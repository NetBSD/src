#	from: @{#}Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.4.8.1 2000/08/07 01:50:02 mrg Exp $

.include <bsd.own.mk>

.if (${MACHINE_ARCH} != "sparc64")
PROG=		g++
SRCS=		gcc.c g++spec.c prefix.c version.c choose-temp.c pexecute.c \
		mkstemp.c obstack.c
BINDIR=		/usr/bin
CPPFLAGS+=	-DLANG_SPECIFIC_DRIVER
CPPFLAGS+=	-I${DIST}/cp

LINKS+=		${BINDIR}/g++ ${BINDIR}/c++
.endif

MAN=		g++.1
MLINKS=		g++.1 c++.1

.include <bsd.prog.mk>

.PATH:		${DIST}/cp
