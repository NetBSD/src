#	$NetBSD: Makefile,v 1.2 2010/07/18 10:58:07 mrg Exp $

SUBDIR= atom property event aux .WAIT \
	reply image icccm keysyms render-util

.include <bsd.subdir.mk>

INCSDIR=${X11INCDIR}/xcb
INCS=	xcb_atom.h xcb_aux.h xcb_bitops.h xcb_event.h xcb_icccm.h \
	xcb_image.h xcb_keysyms.h xcb_pixel.h xcb_property.h \
	xcb_renderutil.h xcb_reply.h

.include <bsd.inc.mk>

.PATH:	${X11SRCDIR.xcb-util}/atom
.PATH:	${X11SRCDIR.xcb-util}/aux
.PATH:	${X11SRCDIR.xcb-util}/event
.PATH:	${X11SRCDIR.xcb-util}/icccm
.PATH:	${X11SRCDIR.xcb-util}/image
.PATH:	${X11SRCDIR.xcb-util}/keysyms
.PATH:	${X11SRCDIR.xcb-util}/property
.PATH:	${X11SRCDIR.xcb-util}/renderutil
.PATH:	${X11SRCDIR.xcb-util}/reply
