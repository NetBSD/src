#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:08 lukem Exp $

.include <bsd.own.mk>

LIB=	dpstk
DIST=	${X11SRCDIR.xc}/lib/${LIB}
.PATH:	${DIST}

SRCS=	XDPSpreview.c XDPSpwraps.c XDPSshare.c XDPSswraps.c XDPSuserpath.c

CPPFLAGS+=	-DDEFAULT_RESOURCE_PATH=\"${RESPATH}\" -I.

DPSRCS+=	XDPSpwraps.h XDPSswraps.h
CLEANFILES+=	XDPSpwraps.c XDPSpwraps.h XDPSswraps.c XDPSswraps.h


.include "${NETBSDSRCDIR}/x11/tools/pswrap/Makefile.pswrap"

#LM#	LDADD+=	-lXt -lSM -lICE -ldps -lXext -lX11
#LM#	DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${DPSLIB} ${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
