#	$NetBSD: Makefile,v 1.6 2017/01/04 15:44:06 christos Exp $

.include <bsd.own.mk>

PROG=	xinput
SRCS=	buttonmap.c feedback.c hierarchy.c list.c property.c setcp.c setint.c \
	setmode.c setptr.c state.c test.c test_xi2.c transform.c xinput.c

CPPFLAGS+=	-DVERSION='"1.6.2"'
CPPFLAGS+=	-DHAVE_XI2 -DHAVE_XI22

LDADD+=	-lXi -lX11 -lXrandr -lXinerama -lm
DPADD+=	${LIBXI} ${LIBM}
.PATH:	${X11SRCDIR.${PROG}}/src
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
