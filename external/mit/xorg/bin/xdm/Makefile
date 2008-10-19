#	$NetBSD: Makefile,v 1.3.2.2 2008/10/19 22:41:00 haad Exp $

.include <bsd.own.mk>

PROG=		xdm
SRCS=		access.c auth.c choose.c daemon.c dm.c dpylist.c \
		error.c file.c genauth.c mitauth.c netaddr.c policy.c \
		protodpy.c reset.c resource.c server.c session.c socket.c \
		streams.c util.c xdmauth.c xdmcp.c \
		Login.c greet.c verify.c

CPPFLAGS+=		-DRETSIGTYPE=void
CPPFLAGS.auth.c=	-DBSD44SOCKETS
CPPFLAGS.socket.c=	-DBSD44SOCKETS
CPPFLAGS.xdmcp.c=	-DBSD44SOCKETS
CPPFLAGS.xdmshell.c=	-DHAS_VFORK
CPPFLAGS.resource.c=	\
	-DDEF_SERVER_LINE="\":0 local ${X11BINDIR}/X :0\"" \
	-DXRDB_PROGRAM=\"${X11BINDIR}/xrdb\" \
	-DDEF_SESSION="\"${X11BINDIR}/xterm -ls\"" \
	-DDEF_USER_PATH=\"/bin:/usr/bin:/usr/pkg/bin:/usr/local/bin:${X11BINDIR}\" \
	-DDEF_SYSTEM_PATH=\"/sbin:/usr/sbin:/bin:/usr/bin:${X11BINDIR}\" \
	-DDEF_SYSTEM_SHELL=\"/bin/sh\" \
	-DDEF_FAILSAFE_CLIENT=\"${X11BINDIR}/xterm\" \
	-DDEF_XDM_CONFIG=\"${XDMDIR}/xdm-config\" \
	-DDEF_AUTH_DIR=\"${XDMVARDIR}\" \
	-DDEF_GREETER_LIB=\"${XDMGREETERLIB}\"

LDADD+=		-lXau -lXdmcp -lXinerama -lXpm -lXmu
LDADD+=		-lXt -lSM -lICE -lXext -lX11 -lcrypt
DPADD+=		${LIBXAU} ${LIBXDMCP} ${LIBXINERAMA} ${LIBXPM} ${LIBXMU}
DPADD+=		${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBCRYPT}

SUBDIR=		chooser config
TARGETS+=	configinstall

.PATH:		${X11SRCDIR.${PROG}}
.PATH:		${X11SRCDIR.${PROG}}/greeter

BUILDSYMLINKS+=	xdm.man.cpp xdm.man

.include "Makefile.xdm"

CPPFLAGS.resource.c+=	-DDEF_CHOOSER=\"${XDMCHOOSERPATH}\"

X11MANCPP=	yes
X11EXTRAMANDEFS+=	-DARC4_RANDOM \
			-DCHOOSERPATH=${XDMCHOOSERPATH} \
			-DGREETERLIBPATH=${XDMGREETERLIB} \
			-DXDMDIR=${XDMDIR} \
			-DXDMAUTHDIR=${XDMVARDIR} \
			-DXDMLOGDIR=${XDMLOGDIR} \
			-DXDMPIDDIR=${XDMPIDDIR}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
