#	$NetBSD: Makefile,v 1.4 2004/08/04 08:37:05 lukem Exp $

.include <bsd.own.mk>

LIB=	dpstk
DIST=	${X11SRCDIR.xc}/lib/${LIB}
.PATH:	${DIST}

SRCS=	XDPSpreview.c XDPSpwraps.c XDPSshare.c XDPSswraps.c XDPSuserpath.c

CPPFLAGS+=	-DDEFAULT_RESOURCE_PATH=\"${RESPATH}\" -I.

DPSRCS+=	XDPSpwraps.h XDPSswraps.h
CLEANFILES+=	XDPSpwraps.c XDPSpwraps.h XDPSswraps.c XDPSswraps.h


.include "${NETBSDSRCDIR}/x11/tools/pswrap/Makefile.pswrap"

LIBDPLIBS=\
	Xt ../Xt \
	SM ../SM \
	ICE ../ICE \
	dps ../dps \
	Xext ../Xext \
	X11 ../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
