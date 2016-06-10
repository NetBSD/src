#	$NetBSD: Makefile,v 1.1.1.1 2016/06/10 03:42:13 mrg Exp $

LIBISPRIVATE=	yes
LIB=		Xext

.include "../Makefile.serverlib"

.include "Makefile.Xextbuiltin"
.include "Makefile.Xextmodule"

SRCS=	${SRCS.Xextbuiltin} ${SRCS.Xextmodule}

SUBDIR=	Xextbuiltin Xextmodule

.include <bsd.subdir.mk>
.include "Makefile.Xext"
