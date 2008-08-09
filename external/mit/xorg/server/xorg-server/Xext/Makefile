#	$NetBSD: Makefile,v 1.4 2008/08/09 05:26:41 rtr Exp $

LIBISPRIVATE=	yes
LIB=		Xext

.include "../Makefile.serverlib"

.include "Makefile.Xextbuiltin"
.include "Makefile.Xextmodule"
.include "Makefile.Xext"

SRCS=	${SRCS.Xext} ${SRCS.Xextbuiltin} ${SRCS.Xextmodule}

SUBDIR=	Xextbuiltin Xextmodule

.include <bsd.subdir.mk>
