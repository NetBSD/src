#	$NetBSD: Makefile,v 1.27.2.1 2017/04/21 16:53:00 bouyer Exp $

.include <bsd.own.mk>

.include "Makefile.common"

SUBDIR=		doc include .WAIT
SUBDIR+=	glx
SUBDIR+=	dix fb mi Xext miext os randr render 
SUBDIR+=	present pseudoramiX
SUBDIR+=	Xi xkb xkbstubs dbe record xfixes
SUBDIR+=	damageext composite config .WAIT

# XXX
.if ${HAVE_DRI3:Uno} != "no"
SUBDIR+=	dri3
.endif

.if ${HAVE_XORG_GLAMOR} != "no"
SUBDIR+=	glamor
.endif

# And finally, build the servers.
SUBDIR+=	.WAIT hw

.PATH:	${X11SRCDIR.xorg-server}

PKGCONFIG=	xorg-server
# Use -fvisibility=hidden for symbol_visibility?
PKGCONFIG_SED_FLAGS= \
	-e "s,@sysconfigdir@,${X11ETCDIR},; \
	    s,@XORG_DRIVER_LIBS@,,; \
	    s,@SDK_REQUIRED_MODULES@,xproto >= 7.0.28 randrproto >= 1.5.0 renderproto >= 0.11 xextproto >= 7.2.99.901 inputproto >= 2.3 kbproto >= 1.0.3 fontsproto >= 2.1.3 fixesproto >= 5.0 damageproto >= 1.1 xcmiscproto >= 1.2.0 bigreqsproto >= 1.1.0 xtrans >= 1.3.5 presentproto >= 1.0,; \
	    s,@symbol_visibility@,,"

.include <bsd.x11.mk>
.include <bsd.man.mk>
.include <bsd.subdir.mk>
