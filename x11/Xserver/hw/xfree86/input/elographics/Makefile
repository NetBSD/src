#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:41 rtr Exp $

XMODULE=	elographics

.include "../../../../Makefile.serverlib"
.include "Makefile.elographics"

SRCS+=		${SRCS.elographics}
CPPFLAGS+=	${CPPFLAGS.elographics} ${X11FLAGS.LOADABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/input
.include "../../Makefile.drivermod"
