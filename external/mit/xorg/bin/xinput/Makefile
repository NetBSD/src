#	$NetBSD: Makefile,v 1.3 2009/03/10 04:50:24 mrg Exp $

.include <bsd.own.mk>

PROG=	xinput
SRCS=	buttonmap.c feedback.c list.c property.c setint.c setmode.c \
	setptr.c state.c test.c version.c xinput.c \

CPPFLAGS+=	-DVERSION='"1.4.0"'

LDADD+=	-lXi -lm
DPADD+=	${LIBXI} ${LIBM}
.PATH:	${X11SRCDIR.${PROG}}/src
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
