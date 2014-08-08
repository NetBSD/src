#	$NetBSD: Makefile,v 1.3 2014/08/08 23:56:57 joerg Exp $

.include "../../Makefile.inc"

BINDIR=	/usr/bin
PROG=	ekermit
SRCS=	main.c kermit.c unixio.c

MAN=	ekermit.1

.PATH: ${EKERMITDIST}

CWARNFLAGS+=	-Wno-error=pointer-sign

.include <bsd.prog.mk>
