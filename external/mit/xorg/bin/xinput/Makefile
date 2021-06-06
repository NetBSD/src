#	$NetBSD: Makefile,v 1.6.22.1 2021/06/06 20:30:46 cjep Exp $

.include <bsd.own.mk>

PROG=	xinput
SRCS=	buttonmap.c feedback.c hierarchy.c list.c property.c setcp.c setint.c \
	setmode.c setptr.c state.c test.c test_xi2.c transform.c xinput.c

CPPFLAGS+=	-DVERSION='"1.6.2"'
CPPFLAGS+=	-DHAVE_XI2 -DHAVE_XI22

LDADD+=	${XLIBLDADD_XI} ${XLIBLDADD_XRANDR} -lXinerama -lm
DPADD+=	${XLIBDPADD_XI} ${XLIBDPADD_XRANDR} ${LIBXINERAMA} ${LIBM}
.PATH:	${X11SRCDIR.${PROG}}/src
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
