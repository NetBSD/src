#	$NetBSD: Makefile,v 1.6.2.2 2008/09/18 05:18:43 wrstuden Exp $

LIBISPRIVATE=	yes
LIB=		Xextmodule

COPTS+=	${CPICFLAGS}

.include "../../Makefile.serverlib"
.include "../Makefile.Xextmodule"

SRCS=	${SRCS.Xextmodule} shape.c ${SRCS.Xext}

.include "../Makefile.Xext"
