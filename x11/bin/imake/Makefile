#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:06 lukem Exp $

.include <bsd.own.mk>

PROG=	imake

CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/X11

CPP_PROGRAM=	cpp
CPPFLAGS+=	-DCPP_PROGRAM=\"$(CPP_PROGRAM)\"
X11EXTRAMANDEFS=-D__cpp__=$(CPP_PROGRAM)

.PATH:	${X11SRCDIR.xc}/config/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
