#	$NetBSD: Makefile,v 1.1 2004/01/06 14:12:36 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/fonts

.include "../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	speedo
.PATH:	${X11SRCDIR.xc}/lib/font/Speedo
.PATH:	${X11SRCDIR.xc}/lib/font/Speedo/module

SRCS=	spfuncs.c spfile.c spinfo.c sperr.c spfont.c spglyph.c spencode.c \
	bics-unicode.c do_char.c do_trns.c out_bl2d.c out_blk.c out_scrn.c \
	out_util.c reset.c set_spcs.c set_trns.c speedomod.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DFONTMODULE
		
.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
