#	$NetBSD: Makefile,v 1.2.4.1 2004/11/15 09:12:42 rtr Exp $

XMODULE=	spaceorb

.include "../../../../Makefile.serverlib"
.include "Makefile.spaceorb"

SRCS+=		${SRCS.spaceorb}
CPPFLAGS+=	${CPPFLAGS.spaceorb} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/input
.include "../../Makefile.drivermod"
