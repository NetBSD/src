#	$NetBSD: Makefile,v 1.4.36.1 2021/06/02 14:46:21 cjep Exp $

.include <bsd.own.mk>

PROG=	xgc
SRCS=	dashlist.c planemask.c getfile.c tests.c text.c \
	choice.c main.c interpret.c record.c testfrac.c gram.y lex.l

YHEADER=1
CPPFLAGS+=-I. -I${X11SRCDIR.${PROG}}

APPDEFS=Xgc Xgc-color

LDADD+=	${XLIBLDADD_XAW} -lm
DPADD+=	${XLIBDPADD_XAW} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
