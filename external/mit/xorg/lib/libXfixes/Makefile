#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

LIB=	Xfixes

XFIXESDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XFIXESDIR}/src
.PATH:		${XFIXESDIR}/include/X11/extensions
SRCS=		Cursor.c Region.c SaveSet.c Selection.c Xfixes.c

INCS=	Xfixes.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/extensions

.if 0
FILES=			xfixes.pc
FILESDIR_xfixes.pc=	${X11USRLIBDIR}/pkgconfig

CLEANFILES+=	${FILES}
realall:	${FILES}

XCURSORDIR=	${X11SRCDIR.xc}/lib/Xcursor
CONFIG_SUBST=	${HOST_SH} ${XCURSORDIR}/config-subst \
		prefix="${X11ROOTDIR}" \
		exec_prefix="${X11BINDIR}" \
		libdir="${X11USRLIBDIR}" \
		includedir="${X11INCDIR}" \
		hardcode_libdir_flag_spec="-Wl,-rpath,${X11USRLIBDIR}" \
		X_LIBS="${X11USRLIBDIR} -lX11" \
		C_FLAGS="${X11INCDIR} ${X11FLAGS.THREADLIB}" \
		VERSION="3.0.0"

xfixes.pc:
	${CONFIG_SUBST} < ${XFIXESDIR}/xfixes.pc.in > ${.TARGET}
.endif

.include <bsd.x11.mk>
.include <bsd.lib.mk>
