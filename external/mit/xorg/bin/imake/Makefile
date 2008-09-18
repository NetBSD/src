#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:26 wrstuden Exp $

.include <bsd.own.mk>

PROG=	imake

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11

CPP_PROGRAM=	cpp
CPPFLAGS+=	-DCPP_PROGRAM=\"$(CPP_PROGRAM)\"
X11EXTRAMANDEFS=-D__cpp__=$(CPP_PROGRAM)

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
