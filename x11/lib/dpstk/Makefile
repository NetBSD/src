#	$NetBSD: Makefile,v 1.2 2003/09/12 08:08:21 lukem Exp $

.include <bsd.own.mk>

LIB=	dpstk
DIST=	${X11SRCDIR.xc}/lib/${LIB}
.PATH:	${DIST}

SRCS=	XDPSpreview.c XDPSpwraps.c XDPSshare.c XDPSswraps.c XDPSuserpath.c

CPPFLAGS+=	-DDEFAULT_RESOURCE_PATH=\"${RESPATH}\" -I.

DPSRCS+=	XDPSpwraps.h XDPSswraps.h
CLEANFILES+=	XDPSpwraps.c XDPSpwraps.h XDPSswraps.c XDPSswraps.h


.include "${NETBSDSRCDIR}/x11/tools/pswrap/Makefile.pswrap"

LDADD+=	-lXt -lSM -lICE -ldps -lXext -lX11
DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${DPSLIB} ${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
