#	$NetBSD: Makefile,v 1.2.2.1 2004/11/15 09:12:41 rtr Exp $

XMODULE=	wsfb

.include "../../../../Makefile.serverlib"
.include "Makefile.wsfb"

SRCS+=		${SRCS.wsfb}
CPPFLAGS+=	${CPPFLAGS.wsfb} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
