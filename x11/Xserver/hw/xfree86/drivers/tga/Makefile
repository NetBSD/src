#	$NetBSD: Makefile,v 1.2.4.1 2004/11/15 09:12:41 rtr Exp $

XMODULE=	tga

.include "../../../../Makefile.serverlib"
.include "Makefile.tga"

SRCS+=		${SRCS.tga}
CPPFLAGS+=	${CPPFLAGS.tga} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
