#	$NetBSD: Makefile,v 1.6.4.1 2014/05/22 14:09:46 yamt Exp $

LIBISPRIVATE=	yes
LIB=		Xextmodule

COPTS+=	${PICFLAGS}

.include "../../Makefile.serverlib"
.include "../Makefile.Xextmodule"

SRCS=	${SRCS.Xextmodule} shape.c ${SRCS.Xext}

.include "../Makefile.Xext"
