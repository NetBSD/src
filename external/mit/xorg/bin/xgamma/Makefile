#	$NetBSD: Makefile,v 1.3.24.1 2021/06/06 20:30:46 cjep Exp $

.include <bsd.own.mk>

PROG=	xgamma

LDADD+=	-lXxf86vm ${XLIBLDADD_XAW} # XXX xf86vm
DPADD+=	${LIBXXF86VM} ${LIBDPADD_XAW}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
