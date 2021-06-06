#	$NetBSD: Makefile,v 1.2.36.1 2021/06/06 20:30:45 cjep Exp $

.include <bsd.own.mk>

PROG=	sxpm

CPPFLAGS+=	-I${X11SRCDIR.Xpm}/src
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11

LDADD+=	${XLIBLDADD_XPM} ${XLIBLDADD_XT}
DPADD+=	${XLIBDPADD_XPM} ${XLIBDPADD_XT}

.PATH:	${X11SRCDIR.Xpm}/${PROG}
.PATH:	${X11SRCDIR.Xpm}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
