#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:54 lukem Exp $

NOPIC=	1

.include <bsd.own.mk>

LIB=	fntstubs
.PATH:	${X11SRCDIR.xc}/lib/font/stubs

SRCS=	cauthgen.c csignal.c delfntcid.c errorf.c fatalerror.c \
	findoldfnt.c getcres.c getdefptsize.c getnewfntcid.c gettime.c \
	initfshdl.c regfpefunc.c rmfshdl.c servclient.c setfntauth.c \
	stfntcfnt.c xpstubs.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/lib/font/include \
		${X11FLAGS.SERVER}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
