#	$NetBSD: Makefile,v 1.2.2.2 2008/09/18 05:18:37 wrstuden Exp $

.include <bsd.own.mk>

LIB=	Xdamage

XDAMAGEDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XDAMAGEDIR}/src
.PATH:		${XDAMAGEDIR}/include/X11/extensions
SRCS=		Xdamage.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/extensions

INCS=	Xdamage.h
INCSDIR=${X11INCDIR}/X11/extensions

.if 0
FILES=			xdamage.pc
FILESDIR_xdamage.pc=	${X11USRLIBDIR}/pkgconfig

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
		VERSION="1.0.1"

xdamage.pc:
	${CONFIG_SUBST} < ${XDAMAGEDIR}/xdamage.pc.in > ${.TARGET}
.endif

.include <bsd.x11.mk>
.include <bsd.lib.mk>
