#	$NetBSD: Makefile,v 1.2.2.2 2008/10/19 22:40:59 haad Exp $

.include <bsd.own.mk>

PROG=	twm
SRCS=	gram.y lex.l deftwmrc.c add_window.c gc.c list.c twm.c \
	parse.c menus.c events.c resize.c util.c version.c iconmgr.c \
	cursor.c icons.c session.c

YHEADER=1

TWMCONFIGDIR=		${X11ETCDIR}/twm

X11EXTRAMANDEFS+=	-e 's,TWMDIR,${TWMCONFIGDIR},g'

TWMDIR=			${X11SRCDIR.${PROG}}

CPPFLAGS+=		-I. -I${TWMDIR}/src
CPPFLAGS+=		-DHAS_MKSTEMP -DXORG_RELEASE=${XORG_RELEASE} \
			-DXVENDORNAME=${XVENDORNAME}

CPPFLAGS.parse.c=	-DSYSTEM_INIT_FILE=\"${TWMCONFIGDIR}/system.twmrc\"

CONFIGFILES=	system.twmrc
FILESDIR=	${TWMCONFIGDIR}

DPSRCS+=	deftwmrc.c
CLEANFILES+=	deftwmrc.c
deftwmrc.c: system.twmrc deftwmrc.sed
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	echo '/* ' >>$@
	echo ' * This file is generated automatically from the default' >>$@
	echo ' * twm bindings file system.twmrc by the twm Makefile.' >>$@
	echo ' */' >>$@
	echo 'char *defTwmrc[] = {' >>$@
	sed -f ${TWMDIR}/src/deftwmrc.sed \
	     < ${TWMDIR}/src/system.twmrc >>$@
	echo '    (char *) 0 };' >>$@


LDADD+=	-lXmu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${TWMDIR}/src ${TWMDIR}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
