#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:40 rtr Exp $

XMODULE=	i740

.include "../../../../Makefile.serverlib"
.include "Makefile.i740"

SRCS+=		${SRCS.i740}
CPPFLAGS+=	${CPPFLAGS.i740} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
