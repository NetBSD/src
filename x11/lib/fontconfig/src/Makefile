#	$NetBSD: Makefile,v 1.5.2.1 2004/04/29 06:08:36 jmc Exp $

.include <bsd.own.mk>

LIB=	fontconfig
.PATH:	${X11SRCDIR.xc}/extras/fontconfig/src

SRCS=	fcatomic.c fcblanks.c fccache.c fccfg.c fccharset.c fcdbg.c \
	fcdefault.c fcdir.c fcfreetype.c fcfs.c fcinit.c fclang.c \
	fclist.c fcmatch.c fcmatrix.c fcname.c fcpat.c fcstr.c fcxml.c

LDADD=	-lexpat
DPADD=	${LIBEXPAT}

MAN=	fontconfig.3

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/freetype2 \
		-I${X11SRCDIR.xc}/extras/expat/lib
CPPFLAGS+=	-DFC_DEFAULT_FONTS='"${X11FONTDIR}/Type1"' -DHAVE_EXPAT
CPPFLAGS+=	-DXFREE86_FT2

.include <bsd.x11.mk>
.include <bsd.lib.mk>
