#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:40:01 lukem Exp $

.include <bsd.own.mk>

PROG=	xman
SRCS=	ScrollByL.c handler.c man.c buttons.c help.c \
	search.c globals.c main.c misc.c tkfuncs.c vendor.c

CPPFLAGS+=${X11FLAGS.VERSION}
CPPFLAGS+=-DHAS_SNPRINTF -DHELPFILE=\"${X11LIBDIR}/xman.help\" -DHAS_MKSTEMP
CPPFLAGS+=-DHAS_GROFF -DMANCONFIGSTYLE_OpenBSD -DMANCONF=\"/etc/man.conf\"

APPDEFS=Xman.ad

FILES=	xman.help
FILESDIR=${X11LIBDIR}

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.prog.mk>
.include <bsd.x11.mk>
