#	from: @(#)Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.1.1.1 1998/03/29 11:34:23 mrg Exp $

.if	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "mips")

PROG=           gcc
SRCS=	gcc.c obstack.c version.c pexecute.c choose-temp.c
BINDIR=		/usr/bin

LINKS+=		${BINDIR}/gcc ${BINDIR}/cc 

.endif

MAN=		gcc.1
MLINKS=		gcc.1 cc.1

.include <bsd.prog.mk>
