#	$NetBSD: Makefile,v 1.10.42.1 2012/03/02 18:27:47 riz Exp $

.include <bsd.own.mk>

LIB=	fontconfig
.PATH:	${X11SRCDIR.xc}/extras/fontconfig/src

SRCS=	fcatomic.c fcblanks.c fccache.c fccfg.c fccharset.c fcdbg.c \
	fcdefault.c fcdir.c fcfreetype.c fcfs.c fcinit.c fclang.c \
	fclist.c fcmatch.c fcmatrix.c fcname.c fcpat.c fcstr.c fcxml.c

LIBDPLIBS=\
	freetype	${.CURDIR}/../../freetype2

MAN=	fontconfig.3

CPPFLAGS+=	${X11FLAGS.THREADS}
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2
CPPFLAGS+=	-DFC_DEFAULT_FONTS='"${X11FONTDIR}/Type1"' -DHAVE_EXPAT
CPPFLAGS+=	-DXFREE86_FT2

.include <bsd.x11.mk>
.include <bsd.lib.mk>
