#	$NetBSD: Makefile,v 1.2.4.1 2004/11/15 09:12:39 rtr Exp $

XMODULE=	cirrus_alpine
XMODULE_SUFFIX=.o

.include "../../../../../Makefile.serverlib"
.include "Makefile.cirrus_alpine"

SRCS+=		${SRCS.cirrus_alpine}
CPPFLAGS+=	${CPPFLAGS.cirrus_alpine} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../../Makefile.drivermod"
