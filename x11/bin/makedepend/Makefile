#	$NetBSD: Makefile,v 1.2 2003/09/20 06:20:47 lukem Exp $

.include <bsd.own.mk>

PROG=		makedepend
SRCS=		include.c main.c parse.c pr.c cppsetup.c ifparser.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/config/imake \
		-I${DESTDIR}/${X11INCDIR}/X11

CPPFLAGS.main.c=-DINCLUDEDIR=\"/usr/include\" -DOBJSUFFIX=\".o\"

BUILDSYMLINKS=	mkdepend.man makedepend.man

.PATH:	${X11SRCDIR.xc}/config/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
