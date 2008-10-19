#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:40:58 haad Exp $

.include <bsd.own.mk>

PROG=	dpsinfo
SRCS=	iwraps.c dpsinfo.c

DPSRCS+=	iwraps.c iwraps.h
CLEANFILES+=	iwraps.c iwraps.h
CPPFLAGS+=	-I.

LDADD+=	-ldps -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBDPS} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.include "${NETBSDSRCDIR}/external/mit/xorg/tools/pswrap/Makefile.pswrap"

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
