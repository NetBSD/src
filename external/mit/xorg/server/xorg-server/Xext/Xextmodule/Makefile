#	$NetBSD: Makefile,v 1.6.28.1 2014/08/20 00:00:14 tls Exp $

LIBISPRIVATE=	yes
LIB=		Xextmodule

COPTS+=	${PICFLAGS}

.include "../../Makefile.serverlib"
.include "../Makefile.Xextmodule"

SRCS=	${SRCS.Xextmodule} shape.c ${SRCS.Xext}

.include "../Makefile.Xext"
