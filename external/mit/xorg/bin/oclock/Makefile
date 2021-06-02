#	$NetBSD: Makefile,v 1.3.36.1 2021/06/02 14:46:19 cjep Exp $

.include <bsd.own.mk>

PROG=	oclock
SRCS=	oclock.c Clock.c transform.c

APPDEFS=Clock-color

LDADD+=	${XLIBLDADD_XMU} -lm
DPADD+=	${XLIBDPADD_XMU} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
