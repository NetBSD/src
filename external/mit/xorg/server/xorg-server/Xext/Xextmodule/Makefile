#	$NetBSD: Makefile,v 1.6 2008/08/28 07:33:22 mrg Exp $

LIBISPRIVATE=	yes
LIB=		Xextmodule

COPTS+=	${CPICFLAGS}

.include "../../Makefile.serverlib"
.include "../Makefile.Xextmodule"

SRCS=	${SRCS.Xextmodule} shape.c ${SRCS.Xext}

.include "../Makefile.Xext"
