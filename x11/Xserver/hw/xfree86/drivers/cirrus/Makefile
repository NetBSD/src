#	$NetBSD: Makefile,v 1.4.4.1 2004/11/15 09:12:39 rtr Exp $

XMODULE=	cirrus

.include "../../../../Makefile.serverlib"
.include "Makefile.cirrus"

SRCS+=		${SRCS.cirrus}
CPPFLAGS+=	${CPPFLAGS.cirrus} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"

SUBDIR=	cirrus_alpine cirrus_laguna

.include <bsd.subdir.mk>
