#	$NetBSD: Makefile,v 1.1.1.1 2016/06/10 03:42:13 mrg Exp $

LIBISPRIVATE=	yes
LIB=		Xextmodule

COPTS+=	${PICFLAGS}

.include "../../Makefile.serverlib"
.include "../Makefile.Xextmodule"

SRCS=	${SRCS.Xextmodule} shape.c ${SRCS.Xext}

.include "../Makefile.Xext"
