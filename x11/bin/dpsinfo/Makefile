#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:03 lukem Exp $

.include <bsd.own.mk>

PROG=	dpsinfo
SRCS=	iwraps.c dpsinfo.c

DPSRCS+=	iwraps.c iwraps.h
CLEANFILES+=	iwraps.c iwraps.h
CPPFLAGS+=	-I.

LDADD+=	-ldps -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBDPS} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.include "${NETBSDSRCDIR}/x11/tools/pswrap/Makefile.pswrap"

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
