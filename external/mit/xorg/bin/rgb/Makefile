#	$NetBSD: Makefile,v 1.2.2.2 2008/10/19 22:40:59 haad Exp $

.include <bsd.own.mk>

PROG=	showrgb

CPPFLAGS+=-DNDBM -DRGB_DB=\"${X11LIBDIR}/rgb\" -DUSE_RGB_TXT
CPPFLAGS+=-I${X11SRCDIR.rgb}/include

X11EXTRAMANDEFS+=	-e 's,__RGB_DB__,${X11LIBDIR}/rgb,g; \
			    s,__RGB_DB_TYPE__,text,g'

FILES=	rgb.txt
FILESDIR=${X11LIBDIR}

.PATH:	${X11SRCDIR.rgb}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
