#	$NetBSD: Makefile,v 1.8 2004/08/04 08:37:04 lukem Exp $

.include <bsd.own.mk>

LIB=	Xcursor
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	cursor.c display.c file.c library.c xlib.c

INCS=	Xcursor.h
INCSDIR=${X11INCDIR}/X11/Xcursor

MAN=	Xcursor.3

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-DICONDIR=\"${X11LIBDIR}/icons\"

LIBDPLIBS=\
	Xrender ../Xrender \
	Xext ../Xext \
	X11 ../X11/dynamic

# XXX it would be really swell if SCRIPTS was available from bsd.lib.mk
FILES=			xcursor.pc xcursor-config
FILESDIR_xcursor.pc=	${X11USRLIBDIR}/pkgconfig
FILESDIR_xcursor-config=${X11BINDIR}
FILESOWN_xcursor-config=${BINOWN}
FILESGRP_xcursor-config=${BINGRP}
FILESMODE_xcursor-config=${BINMODE}

CLEANFILES+=${FILES}
realall: ${FILES}

XCURSORDIR=	${X11SRCDIR.xc}/lib/Xcursor
CONFIG_SUBST=	${HOST_SH} ${XCURSORDIR}/config-subst \
		prefix="${X11ROOTDIR}" \
		exec_prefix="${X11BINDIR}" \
		libdir="${X11USRLIBDIR}" \
		includedir="${X11INCDIR}" \
		hardcode_libdir_flag_spec="-Wl,-rpath,${X11USRLIBDIR}" \
		PACKAGE_VERSION="1.0.0"

xcursor.pc:
	${CONFIG_SUBST} < ${XCURSORDIR}/xcursor.pc.in > ${.TARGET}

xcursor-config:
	${CONFIG_SUBST} < ${XCURSORDIR}/xcursor-config.in > ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
