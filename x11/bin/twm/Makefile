#	$NetBSD: Makefile,v 1.7 2008/10/25 22:27:39 apb Exp $

.include <bsd.own.mk>

PROG=	twm
SRCS=	gram.y lex.l deftwmrc.c add_window.c gc.c list.c twm.c \
	parse.c menus.c events.c resize.c util.c version.c iconmgr.c \
	cursor.c icons.c session.c

YHEADER=1

TWMCONFIGDIR=		${X11ETCDIR}/twm

X11EXTRAMANDEFS+=	-DTWMDIR=${TWMCONFIGDIR}

CPPFLAGS+=		-I. -I${X11SRCDIR.xc}/programs/${PROG} -DHAS_MKSTEMP
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
	${TOOL_SED} -f ${X11SRCDIR.xc}/programs/${PROG}/deftwmrc.sed \
	     < ${X11SRCDIR.xc}/programs/${PROG}/system.twmrc >>$@
	echo '    (char *) 0 };' >>$@


LDADD+=	-lXpm -lXmu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXPM} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
