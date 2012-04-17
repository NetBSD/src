#	$NetBSD: Makefile,v 1.1.1.1.4.1 2012/04/17 00:04:48 yamt Exp $

.include <bsd.own.mk>

PROG=	xvinfo

LDADD+=	-lXv -lXext -lX11
DPADD+=	${LIBXV} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

CWARNFLAGS.clang+=	-Wno-dangling-else

.include <bsd.x11.mk>
.include <bsd.prog.mk>
