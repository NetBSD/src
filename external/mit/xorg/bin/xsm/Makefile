#	$NetBSD: Makefile,v 1.2 2010/11/21 06:01:50 mrg Exp $

.include <bsd.own.mk>

PROG=	xsm
SRCS=	auth.c choose.c info.c list.c lock.c log.c \
	mainwin.c misc.c popup.c printhex.c prop.c remote.c \
	restart.c save.c saveutil.c signals.c xsm.c xtwatch.c

XSMDIR=	${X11ETCDIR}/xsm

CPPFLAGS+=-DHAS_MKSTEMP
CPPFLAGS.remote.c=	-DRSHCMD=\"ssh\"
CPPFLAGS.restart.c=	-DSYSTEM_INIT_FILE=\"${XSMDIR}/system.xsm\"

CONFIGFILES=	system.xsm
FILESDIR=	${XSMDIR}

.PATH:	${X11SRCDIR.${PROG}}/app-defaults
APPDEFS=XSm

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
