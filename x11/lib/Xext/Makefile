#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:50:24 lukem Exp $

.include <bsd.own.mk>

LIB=	Xext
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	DPMS.c MITMisc.c XAppgroup.c XEVI.c XLbx.c XMultibuf.c XSecurity.c \
	XShape.c XShm.c XSync.c XTestExt1.c Xcup.c Xdbe.c extutil.c globals.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${X11SRCDIR.xc}/lib/X11

#LM#	LDADD+=	-lX11
#LM#	DPADD+=	${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
