#	$NetBSD: Makefile,v 1.1 2003/12/26 05:23:15 rtr Exp $

.include <bsd.own.mk>

PROG=	xf86config

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf86config

SRCS=	xf86config.c cards.c

LDADD=	-lxkbfile -lXext -lX11
DPADD=	${LIBXKBFILE} ${LIBXEXT} ${LIBX11}

CPPFLAGS+=	-DCARD_DATABASE_FILE='"${X11INCDIR}/X11/Cards"' \
		-DXKB_RULES_DIR=\"${X11INCDIR}/X11/xkb/rules\" \
		-DXCONFIGFILE=\"XF86Config\" \
		-DXVERSION="(((4) * 10000000) + (( 3) * 100000) + (( 0) * 1000) + 0)" \
		-DWSCONS_SUPPORT

.if ${MACHINE} == "i386"
CPPFLAGS+=	-DPCCONS_SUPPORT -DPCVT_SUPPORT
.endif

.include <bsd.x11.mk>
.include <bsd.prog.mk>
