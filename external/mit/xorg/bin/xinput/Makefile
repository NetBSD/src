#	$NetBSD: Makefile,v 1.2 2008/09/06 06:40:29 rtr Exp $

.include <bsd.own.mk>

PROG=	xinput
SRCS=	buttonmap.c feedback.c list.c setint.c setmode.c \
	setptr.c state.c test.c version.c xinput.c

CPPFLAGS+=	-DVERSION='"1.3.0"'

LDADD+=	-lXi -lm
DPADD+=	${LIBXI} ${LIBM}
.PATH:	${X11SRCDIR.${PROG}}/src
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
