#	$NetBSD: Makefile,v 1.8.2.1 2004/11/15 09:12:46 rtr Exp $

NOLINT=	1
.include <bsd.own.mk>

LIB=	fontconfig

.PATH:	${X11SRCDIR.xc}/extras/fontconfig/src
SRCS=	fcatomic.c fcblanks.c fccache.c fccfg.c fccharset.c fcdbg.c \
	fcdefault.c fcdir.c fcfreetype.c fcfs.c fcinit.c fclang.c \
	fclist.c fcmatch.c fcmatrix.c fcname.c fcpat.c fcstr.c fcxml.c

LIBDPLIBS=\
	expat	${.CURDIR}/../../expat

# XXX FONTCONFIG_PATH was arbitrarily chosen
CPPFLAGS+=	-DFONTCONFIG_PATH='"/etc/X11/fonts"' \
		-DFC_DEFAULT_FONTS='"${X11FONTDIR}/Type1"' \
		-DHAVE_EXPAT -DXFREE86_FT2

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2 \
		-I${X11SRCDIR.xc}/extras/expat/lib \
		-I${X11SRCDIR.xc}/extras/freetype2/include

.include <bsd.x11.mk>
.include <bsd.lib.mk>
