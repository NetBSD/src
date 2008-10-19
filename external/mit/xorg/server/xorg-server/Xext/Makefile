#	$NetBSD: Makefile,v 1.5.6.2 2008/10/19 22:41:17 haad Exp $

LIBISPRIVATE=	yes
LIB=		Xext

.include "../Makefile.serverlib"

.include "Makefile.Xextbuiltin"
.include "Makefile.Xextmodule"

SRCS=	${SRCS.Xextbuiltin} ${SRCS.Xextmodule}

SUBDIR=	Xextbuiltin Xextmodule

.include <bsd.subdir.mk>
.include "Makefile.Xext"
