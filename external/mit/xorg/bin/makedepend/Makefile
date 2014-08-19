#	$NetBSD: Makefile,v 1.1.1.1.28.1 2014/08/20 00:00:09 tls Exp $

.include <bsd.own.mk>

PROG=		makedepend
SRCS=		include.c main.c parse.c pr.c cppsetup.c ifparser.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/config/imake \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-DHAVE_RENAME -DHAVE_FCHMOD

CPPFLAGS.main.c=-DINCLUDEDIR=\"/usr/include\" -DOBJSUFFIX=\".o\"

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
