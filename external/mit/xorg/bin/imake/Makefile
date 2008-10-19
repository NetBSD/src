#	$NetBSD: Makefile,v 1.2.2.2 2008/10/19 22:40:57 haad Exp $

.include <bsd.own.mk>

PROG=	imake

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11

CPP_PROGRAM=	cpp
CPPFLAGS+=	-DCPP_PROGRAM=\"$(CPP_PROGRAM)\"
X11EXTRAMANDEFS=-e 's,__cpp__,$(CPP_PROGRAM),g'

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
