#	$NetBSD: Makefile,v 1.3 2003/09/23 11:04:46 lukem Exp $

.include <bsd.own.mk>

PROG=	twm
SRCS=	gram.y lex.l deftwmrc.c add_window.c gc.c list.c twm.c \
	parse.c menus.c events.c resize.c util.c version.c iconmgr.c \
	cursor.c icons.c session.c

YHEADER=1
CPPFLAGS+=		-I. -I${X11SRCDIR.xc}/programs/${PROG} -DHAS_MKSTEMP
CPPFLAGS.parse.c=	-DSYSTEM_INIT_FILE=\"${X11LIBDIR}/twm/system.twmrc\"

FILES=	system.twmrc
FILESDIR=${X11LIBDIR}/twm

DPSRCS+=	deftwmrc.c
CLEANFILES+=	deftwmrc.c
deftwmrc.c: system.twmrc deftwmrc.sed
	rm -f ${.TARGET}
	echo '/* ' >>$@
	echo ' * This file is generated automatically from the default' >>$@
	echo ' * twm bindings file system.twmrc by the twm Makefile.' >>$@
	echo ' */' >>$@
	echo 'char *defTwmrc[] = {' >>$@
	sed -f ${X11SRCDIR.xc}/programs/${PROG}/deftwmrc.sed \
	     < ${X11SRCDIR.xc}/programs/${PROG}/system.twmrc >>$@
	echo '    (char *) 0 };' >>$@


LDADD+=	-lXmu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
