#	$NetBSD: Makefile,v 1.5.2.2 2008/09/18 05:18:43 wrstuden Exp $

LIBISPRIVATE=	yes
LIB=		Xext

.include "../Makefile.serverlib"

.include "Makefile.Xextbuiltin"
.include "Makefile.Xextmodule"

SRCS=	${SRCS.Xextbuiltin} ${SRCS.Xextmodule}

SUBDIR=	Xextbuiltin Xextmodule

.include <bsd.subdir.mk>
.include "Makefile.Xext"
