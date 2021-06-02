#	$NetBSD: Makefile,v 1.4.4.1 2021/06/02 14:46:24 cjep Exp $

.include <bsd.own.mk>

PROG=	xsm
SRCS=	auth.c choose.c info.c list.c lock.c log.c \
	mainwin.c misc.c popup.c printhex.c prop.c remote.c \
	restart.c save.c saveutil.c signals.c xsm.c xtwatch.c

XSMDIR=	${X11ETCDIR}/xsm

CPPFLAGS+=-DHAVE_MKSTEMP -DHAVE_PUTENV
CPPFLAGS.remote.c=	-DRSHCMD=\"ssh\"
CPPFLAGS.restart.c=	-DSYSTEM_INIT_FILE=\"${XSMDIR}/system.xsm\"

CONFIGFILES=	system.xsm
FILESDIR=	${XSMDIR}

.PATH:	${X11SRCDIR.${PROG}}/app-defaults
APPDEFS=XSm

LDADD+=	${XLIBLDADD_XAW}
DPADD+=	${XLIBDPADD_XAW}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

# XXX libXt
COPTS+=		-Wno-incompatible-pointer-types

.include <bsd.x11.mk>
.include <bsd.prog.mk>
