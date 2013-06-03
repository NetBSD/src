#	$NetBSD: Makefile,v 1.1 2013/06/03 23:25:43 mrg Exp $

.include <bsd.own.mk>

LIB=	XdmGreet
.PATH:	${X11SRCDIR.xdm}/greeter

SRCS=	\
	Login.c \
	greet.c \
	verify.c

CPPFLAGS+=	-I${X11SRCDIR.xdm}/include -DRETSIGTYPE=void

.if 0
CPPFLAGS+=	${X11FLAGS.THREADLIB} \
		-DHAVE_XEXTPROTO_71 \
		-DXSERVER_LIBPCIACCESS \
		-DTRUE=1 -DFALSE=0 \
		-I${DESTDIR}${X11INCDIR}/xorg 
.endif

.include <bsd.x11.mk>
.include <bsd.lib.mk>
