#	$NetBSD: Makefile,v 1.5 2004/08/05 01:28:48 lukem Exp $

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
	Xt	${.CURDIR}/../Xt \
	SM	${.CURDIR}/../SM \
	ICE	${.CURDIR}/../ICE \
	dps	${.CURDIR}/../dps \
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
