#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:40 rtr Exp $

XMODULE=	siliconmotion

.include "../../../../Makefile.serverlib"
.include "Makefile.siliconmotion"

SRCS+=		${SRCS.siliconmotion}
CPPFLAGS+=	${CPPFLAGS.siliconmotion} ${X11FLAGS.LODABLE}

.include <bsd.x11.mk>
LIBDIR=	${X11USRLIBDIR}/modules/drivers
.include "../../Makefile.drivermod"
