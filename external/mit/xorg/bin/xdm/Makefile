#	$NetBSD: Makefile,v 1.4.2.1 2009/05/13 18:53:23 jym Exp $

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

LDADD+=		-lXau -lXdmcp -lXft -lXinerama -lXpm
LDADD+=		-lXmu -lXt -lSM -lICE -lXext -lX11
LDADD+=		-lcrypt -lfreetype
DPADD+=		${LIBXAU} ${LIBXDMCP} ${LIBXFT} ${LIBXINERAMA} ${LIBXPM}
DPADD+=		${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}
DPADD+=		${LIBCRYPT} ${LIBFREETYPE}

.if (${USE_PAM} != "no")
LDADD+= -lpam ${PAM_STATIC_LDADD}
DPADD+= ${LIBPAM} ${PAM_STATIC_DPADD}
.endif

SUBDIR=		chooser config
TARGETS+=	configinstall

.PATH:		${X11SRCDIR.${PROG}}
.PATH:		${X11SRCDIR.${PROG}}/greeter

BUILDSYMLINKS+=	xdm.man.cpp xdm.man

.include "Makefile.xdm"

CPPFLAGS.resource.c+=	-DDEF_CHOOSER=\"${XDMCHOOSERPATH}\"
CPPFLAGS+=		-I${DESTDIR}${X11INCDIR}/freetype2

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
