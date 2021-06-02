#	$NetBSD: Makefile,v 1.8.24.1 2021/06/02 14:46:22 cjep Exp $

.include <bsd.own.mk>

PROG=	xlsclients

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR} -DHAVE_STRNLEN

#BUILDSYMLINKS=	xlscli.man xlsclients.man

LDADD+=	${XLIBLDADD_XCB_ATOM}
DPADD+=	${XLIBDPADD_XCB_ATOM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
