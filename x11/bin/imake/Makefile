#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:06 lukem Exp $

.include <bsd.own.mk>

PROG=	imake

CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/X11

CPP_PROGRAM=	cpp
CPPFLAGS+=	-DCPP_PROGRAM=\"$(CPP_PROGRAM)\"
X11EXTRAMANDEFS=-D__cpp__=$(CPP_PROGRAM)

.PATH:	${X11SRCDIR.xc}/config/${PROG}

.include <bsd.prog.mk>
.include <bsd.x11.mk>
