#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:07 lukem Exp $

.include <bsd.own.mk>

PROG=		xdm
SRCS=		access.c auth.c choose.c daemon.c dm.c dpylist.c \
		error.c file.c genauth.c mitauth.c netaddr.c policy.c \
		protodpy.c reset.c resource.c server.c session.c socket.c \
		streams.c util.c xdmauth.c xdmcp.c \
		Login.c greet.c verify.c

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
	-DDEF_XDM_CONFIG=\"${X11LIBDIR}/xdm/xdm-config\" \
	-DDEF_CHOOSER=\"${X11LIBDIR}/xdm/chooser\" \
	-DDEF_AUTH_DIR=\"${X11LIBDIR}/xdm\" \
	-DDEF_GREETER_LIB=\"${X11LIBDIR}/xdm/libXdmGreet.so\"

#FILESDIR=${X11LIBDIR}/etc
#FILES=	xterm.termcap xterm.terminfo
#
#realall: ${FILES}

LDADD+=		-lXau -lXdmcp -lXinerama -lXpm -lXmu
LDADD+=		-lXt -lSM -lICE -lXext -lX11 -lcrypt
DPADD+=		${LIBXAU} ${LIBXDMCP} ${LIBXINERAMA} ${LIBXPM} ${LIBXMU}
DPADD+=		${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBCRYPT}

SUBDIR=		chooser config sessreg
# SUBDIR+=	xdmshell	# XXX

.PATH:		${X11SRCDIR.xc}/programs/${PROG}/greeter

.include "Makefile.xdm"

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
.include <bsd.x11.mk>
