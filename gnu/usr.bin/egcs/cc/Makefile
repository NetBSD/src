#	from: @{#}Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.3 1998/04/03 02:20:29 tv Exp $

PROG=           gcc
SRCS=	gcc.c obstack.c version.c pexecute.c choose-temp.c
BINDIR=		/usr/bin

LINKS+=		${BINDIR}/gcc ${BINDIR}/cc 

MAN=		gcc.1
MLINKS=		gcc.1 cc.1

.include <bsd.prog.mk>
