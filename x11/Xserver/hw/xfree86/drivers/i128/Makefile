#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:40 rtr Exp $

XMODULE=	i128

.include "../../../../Makefile.serverlib"
.include "Makefile.i128"

SRCS+=		${SRCS.i128}
CPPFLAGS+=	${CPPFLAGS.i128} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
