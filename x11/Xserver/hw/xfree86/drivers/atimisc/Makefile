#	$NetBSD: Makefile,v 1.4.2.1 2004/11/15 09:12:39 rtr Exp $

XMODULE=	atimisc

.include "../../../../Makefile.serverlib"
.include "Makefile.atimisc"

SRCS+=		${SRCS.atimisc}
CPPFLAGS+=	${CPPFLAGS.atimisc} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
