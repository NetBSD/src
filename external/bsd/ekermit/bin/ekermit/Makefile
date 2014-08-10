#	$NetBSD: Makefile,v 1.3.2.2 2014/08/10 07:07:02 tls Exp $

.include "../../Makefile.inc"

BINDIR=	/usr/bin
PROG=	ekermit
SRCS=	main.c kermit.c unixio.c

MAN=	ekermit.1

.PATH: ${EKERMITDIST}

CWARNFLAGS+=	-Wno-error=pointer-sign

.include <bsd.prog.mk>
