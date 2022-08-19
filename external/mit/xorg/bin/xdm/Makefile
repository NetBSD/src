#	$NetBSD: Makefile,v 1.19 2022/08/19 15:55:36 nia Exp $

.include <bsd.own.mk>

PROG=		xdm
SRCS=		access.c auth.c choose.c daemon.c dm.c dpylist.c \
		error.c file.c genauth.c mitauth.c netaddr.c policy.c \
		protodpy.c reset.c resource.c server.c session.c socket.c \
		util.c xdmauth.c xdmcp.c prngc.c

.if ${MKPIC} == "no"
SRCS+=		Login.c greet.c verify.c
CPPFLAGS+=	-DSTATIC_GREETER_LIB
.endif

DEF_USER_PATH=	/sbin:/usr/sbin:/bin:/usr/bin:/usr/games:/usr/pkg/sbin:/usr/pkg/bin:/usr/local/sbin:/usr/local/bin:${X11BINDIR}
DEF_SYSTEM_PATH=/sbin:/usr/sbin:/bin:/usr/bin:${X11BINDIR}

CPPFLAGS+=		-DRETSIGTYPE=void
CPPFLAGS.auth.c+=	-DBSD44SOCKETS
CPPFLAGS.socket.c+=	-DBSD44SOCKETS
CPPFLAGS.xdmcp.c+=	-DBSD44SOCKETS
CPPFLAGS.xdmshell.c+=	-DHAS_VFORK
CPPFLAGS.resource.c+=	\
	-DDEF_SERVER_LINE="\":0 local ${X11BINDIR}/X :0\"" \
	-DXRDB_PROGRAM=\"${X11BINDIR}/xrdb\" \
	-DDEF_SESSION="\"${X11BINDIR}/xterm -ls\"" \
	-DDEF_USER_PATH=\"${DEF_USER_PATH}\" \
	-DDEF_SYSTEM_PATH=\"${DEF_SYSTEM_PATH}\" \
	-DDEF_SYSTEM_SHELL=\"/bin/sh\" \
	-DDEF_FAILSAFE_CLIENT=\"${X11BINDIR}/xterm\" \
	-DDEF_XDM_CONFIG=\"${XDMDIR}/xdm-config\" \
	-DDEF_AUTH_DIR=\"${XDMVARDIR}\" \
	-DDEF_GREETER_LIB=\"${XDMGREETERLIB}\"

LDADD+=		-lXau -lXdmcp -lXft -lXinerama -lXpm
LDADD+=		-lXmu -lXt -lSM -lICE -lXext -lXrender -lX11
LDADD+=		-lcrypt -lfreetype -lutil
DPADD+=		${LIBXAU} ${LIBXDMCP} ${LIBXFT} ${LIBXINERAMA} ${LIBXPM}
DPADD+=		${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}
DPADD+=		${LIBCRYPT} ${LIBFREETYPE} ${LIBUTIL}

.if (${USE_PAM} != "no")
LDADD+= -lpam ${PAM_STATIC_LDADD}
DPADD+= ${LIBPAM} ${PAM_STATIC_DPADD}
.endif

SUBDIR+=	chooser config
TARGETS+=	configinstall

.include "Makefile.xdm"

CPPFLAGS.resource.c+=	-DDEF_CHOOSER=\"${XDMCHOOSERPATH}\"

.include "../../xorg-pkg-ver.mk"

X11EXTRAMANDEFS+= \
		-e 's,CHOOSERPATH,${XDMCHOOSERPATH},' \
		-e 's,XDMLOGDIR,${XDMLOGDIR},' \
		-e 's,XDMDIR,${XDMDIR},' \
		-e 's,BINDIR,${X11BINDIR},' \
		-e 's,XDMPIDDIR,${XDMPIDDIR},' \
		-e 's,XDMXAUTHDIR,${XDMVARDIR},' \
		-e 's,DEF_USER_PATH,${DEF_USER_PATH},' \
		-e 's,DEF_SYSTEM_PATH,${DEF_SYSTEM_PATH},' \
		-e 's,DEF_GREETER_LIB,${XDMGREETERLIB},' \
		-e 's,DEV_RANDOM,${DEV_RANDOM},' \
		-e 's,ARC4_RANDOM,1,'

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
