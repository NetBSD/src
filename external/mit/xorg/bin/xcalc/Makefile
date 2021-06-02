#	$NetBSD: Makefile,v 1.5.8.1 2021/06/02 15:03:56 cjep Exp $

.include <bsd.own.mk>

PROG=	xcalc
SRCS=	actions.c math.c xcalc.c

APPDEFS=XCalc XCalc-color

LDADD+=	${XLIBLDADD_XAW} -lm
DPADD+=	${XLIBDPADD_XAW} ${LIBM}

CPPFLAGS+=	-DHAVE_STRLCPY -D_CONST_X_STRING

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
