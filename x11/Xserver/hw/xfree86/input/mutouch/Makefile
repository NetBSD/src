#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:42 rtr Exp $

XMODULE=	mutouch

.include "../../../../Makefile.serverlib"
.include "Makefile.mutouch"

SRCS+=		${SRCS.mutouch}
CPPFLAGS+=	${CPPFLAGS.mutouch} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/input
.include "../../Makefile.drivermod"
