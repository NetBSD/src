#	$NetBSD: Makefile,v 1.3 2004/03/16 17:40:54 matt Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/fonts

.include "../../Makefile.serverlib"

LIB=	speedo
.PATH:	${X11SRCDIR.xc}/lib/font/Speedo
.PATH:	${X11SRCDIR.xc}/lib/font/Speedo/module

SRCS=	spfuncs.c spfile.c spinfo.c sperr.c spfont.c spglyph.c spencode.c \
	bics-unicode.c do_char.c do_trns.c out_bl2d.c out_blk.c out_scrn.c \
	out_util.c reset.c set_spcs.c set_trns.c speedomod.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${XSERVERINCDIR}

.include <bsd.x11.mk>

.if !empty(X11FLAGS.LOADABLE)
CPPFLAGS+=	-DFONTMODULE
LIBDIR=	${XMODULEDIR}
.else
libinstall::
.endif
		
.include <bsd.lib.mk>
