#	$NetBSD: Makefile,v 1.3 2004/05/16 10:25:14 lukem Exp $

.include <bsd.own.mk>

PROG=	xsm
SRCS=	auth.c choose.c info.c list.c lock.c log.c \
	mainwin.c misc.c popup.c printhex.c prop.c remote.c \
	restart.c save.c saveutil.c signals.c xsm.c xtwatch.c

XSMDIR=	${X11ETCDIR}/xsm

CPPFLAGS+=-DHAS_MKSTEMP
CPPFLAGS.remote.c=	-DRSHCMD=\"rsh\"
CPPFLAGS.restart.c=	-DSYSTEM_INIT_FILE=\"${XSMDIR}/system.xsm\"

CONFIGFILES=	system.xsm
FILESDIR=	${XSMDIR}

APPDEFS=XSm.ad

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
