#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:39 rtr Exp $

XMODULE=	apm

.include "../../../../Makefile.serverlib"
.include "Makefile.apm"

SRCS+=		${SRCS.apm}
CPPFLAGS+=	${CPPFLAGS.apm} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
