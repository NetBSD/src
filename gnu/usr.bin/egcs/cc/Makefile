#	from: @{#}Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.5 2000/08/06 15:26:56 mrg Exp $

.include <bsd.own.mk>

.if (${MACHINE_ARCH} != "sparc64")
PROG=           gcc
SRCS=		gcc.c prefix.c version.c choose-temp.c pexecute.c mkstemp.c \
		obstack.c
BINDIR=		/usr/bin

LINKS+=		${BINDIR}/gcc ${BINDIR}/cc 
.endif

MAN=		gcc.1
MLINKS=		gcc.1 cc.1

.include <bsd.prog.mk>
