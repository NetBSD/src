#	$NetBSD: Makefile,v 1.6 2005/09/24 13:51:47 chs Exp $

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
	dps	${.CURDIR}/../dps \
	X11	${.CURDIR}/../X11/dynamic

LDADD+=		-lm
DPADD+=		${LIBM}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
