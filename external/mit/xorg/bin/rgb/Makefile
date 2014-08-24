#	$NetBSD: Makefile,v 1.4 2014/08/24 18:27:16 apb Exp $

.include <bsd.own.mk>

PROG=	showrgb

CPPFLAGS+=-DNDBM -DRGB_DB=\"${X11LIBDIR}/rgb\" -DUSE_RGB_TXT
CPPFLAGS+=-I${X11SRCDIR.rgb}/include

X11EXTRAMANDEFS+=	-e 's,__RGB_DB__,${X11LIBDIR}/rgb,g' \
			-e 's,__RGB_DB_TYPE__,text,g'

FILES=	rgb.txt
FILESDIR=${X11LIBDIR}

.PATH:	${X11SRCDIR.rgb} ${X11SRCDIR.rgb}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
