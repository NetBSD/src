#	$NetBSD: Makefile,v 1.4.4.1 2004/11/15 09:12:40 rtr Exp $

XMODULE=	r128

.include "../../../../Makefile.serverlib"
.include "Makefile.r128"

SRCS+=		${SRCS.r128}
CPPFLAGS+=	${CPPFLAGS.r128} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
