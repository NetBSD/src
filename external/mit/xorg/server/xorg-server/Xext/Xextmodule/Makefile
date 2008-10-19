#	$NetBSD: Makefile,v 1.6.6.2 2008/10/19 22:41:17 haad Exp $

LIBISPRIVATE=	yes
LIB=		Xextmodule

COPTS+=	${CPICFLAGS}

.include "../../Makefile.serverlib"
.include "../Makefile.Xextmodule"

SRCS=	${SRCS.Xextmodule} shape.c ${SRCS.Xext}

.include "../Makefile.Xext"
