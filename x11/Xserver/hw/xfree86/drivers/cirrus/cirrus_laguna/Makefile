#	$NetBSD: Makefile,v 1.2.4.1 2004/11/15 09:12:40 rtr Exp $

XMODULE=	cirrus_laguna
XMODULE_SUFFIX=.o

.include "../../../../../Makefile.serverlib"
.include "Makefile.cirrus_laguna"

SRCS+=		${SRCS.cirrus_laguna}
CPPFLAGS+=	${CPPFLAGS.cirrus_laguna} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../../Makefile.drivermod"
