#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:11 lukem Exp $

.include <bsd.own.mk>

PROG=	texteroids
SRCS=	twraps.c texteroids.c

DPSRCS+=	twraps.c twraps.h
CLEANFILES+=	twraps.c twraps.h
CPPFLAGS+=	-I.

LDADD+=	-ldps -lXt -lSM -lICE -lXext -lX11 -lm
DPADD+=	${LIBDPS} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBM}

.include "${NETBSDSRCDIR}/x11/tools/pswrap/Makefile.pswrap"

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
