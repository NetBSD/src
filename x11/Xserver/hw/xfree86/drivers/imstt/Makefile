#	$NetBSD: Makefile,v 1.2.4.1 2004/11/15 09:12:40 rtr Exp $

XMODULE=	imstt

.include "../../../../Makefile.serverlib"
.include "Makefile.imstt"

SRCS+=		${SRCS.imstt}
CPPFLAGS+=	${CPPFLAGS.imstt} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
