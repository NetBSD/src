#	$NetBSD: Makefile,v 1.3.36.1 2021/06/02 14:46:17 cjep Exp $

.include <bsd.own.mk>

PROG=	editres
SRCS=	actions.c comm.c editres.c geometry.c handler.c setvalues.c \
	svpopup.c utils.c widgets.c wtree.c

APPDEFS=Editres.ad Editres-color.ad

LDADD+=	${XLIBLDADD_XAW}
DPADD+=	${XLIBDPADD_XAW}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

CWARNFLAGS.clang+=	-Wno-format

.include <bsd.x11.mk>
.include <bsd.prog.mk>
