#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:50:50 lukem Exp $

.include <bsd.own.mk>

LIB=	fontconfig
.PATH:	${X11SRCDIR.xc}/lib/fontconfig/src

SRCS=	fcatomic.c fcblanks.c fccache.c fccfg.c fccharset.c fcdbg.c \
	fcdefault.c fcdir.c fcfreetype.c fcfs.c fcinit.c fclang.c \
	fclist.c fcmatch.c fcmatrix.c fcname.c fcpat.c fcstr.c fcxml.c

MAN=	fontconfig.3

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/freetype2
CPPFLAGS+=	-DFC_DEFAULT_FONTS='"${X11FONTDIR}/Type1"' -DHAVE_EXPAT

.include <bsd.lib.mk>
.include <bsd.x11.mk>
