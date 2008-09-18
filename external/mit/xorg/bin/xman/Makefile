#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:31 wrstuden Exp $

.include <bsd.own.mk>

PROG=	xman
SRCS=	ScrollByL.c handler.c man.c buttons.c help.c \
	search.c globals.c main.c misc.c tkfuncs.c vendor.c

CPPFLAGS+=${X11FLAGS.VERSION}
CPPFLAGS+=-DHAS_SNPRINTF -DHELPFILE=\"${X11LIBDIR}/xman.help\" -DHAS_MKSTEMP
CPPFLAGS+=-DHAS_GROFF -DMANCONFIGSTYLE_OpenBSD -DMANCONF=\"/etc/man.conf\"

APPDEFS=Xman.ad

BUILDSYMLINKS=	Xman-xprint.ad Xman.ad

FILES=	xman.help
FILESDIR=${X11LIBDIR}

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
