#	$NetBSD: Makefile,v 1.8.4.1 2021/06/06 20:30:45 cjep Exp $

.include <bsd.own.mk>

PROG=	xauth
SRCS=	xauth.c gethost.c process.c parsedpy.c

CPPFLAGS+=		-DRETSIGTYPE=void -DPACKAGE_VERSION=\"1.0.8\"
CPPFLAGS+= 		-DHAVE_STRLCPY
CPPFLAGS.gethost.c=	${X11FLAGS.CONNECTION}
CPPFLAGS.parsedpy.c=	${X11FLAGS.CONNECTION}

.if (${USE_INET6} != "no")
CPPFLAGS+=      	-DIPv6
.endif

CWARNFLAGS.clang=	-Wno-empty-body
COPTS.parsedpy.c+=	${GCC_NO_STRINGOP_OVERFLOW}

LDADD+=	${XLIBLDADD_XAU} ${XLIBLDADD_XMUU}
DPADD+= ${XLIBDPADD_XAU} ${XLIBDPADD_XMUU}
#DPADD+=	${LIBXAU} ${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

.include <bsd.own.mk>

USE_SSP:=	no

.include <bsd.x11.mk>
.include <bsd.prog.mk>
