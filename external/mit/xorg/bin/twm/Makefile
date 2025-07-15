#	$NetBSD: Makefile,v 1.10 2025/07/15 04:20:42 mrg Exp $

.include <bsd.own.mk>

PROG=	twm
SRCS=	gram.y lex.l deftwmrc.c add_window.c gc.c list.c twm.c \
	parse.c menus.c events.c resize.c util.c iconmgr.c \
	cursor.c icons.c session.c

YHEADER=1

TWMCONFIGDIR=		${X11ETCDIR}/twm

X11EXTRAMANTRANSFORMS+=	__datadir__/X11/twm ${TWMCONFIGDIR}

TWMDIR=			${X11SRCDIR.${PROG}}

CPPFLAGS+=		-I. -I${TWMDIR}/src
CPPFLAGS+=		-DHAVE_MKSTEMP \
			-DHAVE_XRANDR \
			-DXORG_RELEASE=${XORG_RELEASE} \
			-DXVENDORNAME=${XVENDORNAME} \
			-DAPP_NAME=\"twm\" \
			-DAPP_CLASS=\"twm\" \
			-DAPP_VERSION=\"$(VERSION)\"

CPPFLAGS.parse.c=	-DSYSTEM_INIT_FILE=\"${TWMCONFIGDIR}/system.twmrc\"

CONFIGFILES=	system.twmrc
FILESDIR=	${TWMCONFIGDIR}

DPSRCS+=	deftwmrc.c
CLEANFILES+=	deftwmrc.c
# XXX deftwmrc.sed deleted in twm 1.0.13.  copied here now.  consider
# replaceing this with new gen_deftwmrc.sh.
deftwmrc.c: system.twmrc deftwmrc.sed
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	echo '/* ' >>$@
	echo ' * This file is generated automatically from the default' >>$@
	echo ' * twm bindings file system.twmrc by the twm Makefile.' >>$@
	echo ' */' >>$@
	echo 'const unsigned char *defTwmrc[] = {' >>$@
	${TOOL_SED} -f ${.CURDIR}/deftwmrc.sed \
	     < ${TWMDIR}/src/system.twmrc >>$@
	echo '    (char *) 0 };' >>$@


LDADD+=	-lXrandr -lXmu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXRANDR} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${TWMDIR}/src ${TWMDIR}/man

CWARNFLAGS.clang+=	-Wno-pointer-sign

# XXX: has const issues best solved upstream
COPTS.menus.c+=	-Wno-error
COPTS.util.c+=	-Wno-error

.include <bsd.x11.mk>
.include <bsd.prog.mk>

# Needs bsd.x11.mk
VERSION!=		${PRINT_PACKAGE_VERSION} ${X11SRCDIR.twm}/configure
