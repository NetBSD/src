#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

LIB=	Xevie

XEVIEDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XEVIEDIR}/src
.PATH:		${XEVIEDIR}/include/X11/extensions
SRCS=		Xevie.c

INCS=	Xevie.h
INCSDIR=${X11INCDIR}/X11/extensions

.if 0
FILES=		xevie.pc
FILESDIR_xevie.pc=	${X11USRLIBDIR}/pkgconfig

CLEANFILES+=	${FILES}
realall:	${FILES}

XCURSORDIR=	${X11SRCDIR.xc}/lib/Xcursor
CONFIG_SUBST= 	${HOST_SH} ${XCURSORDIR}/config-subst \
		prefix="${X11ROOTDIR}" \
		exec_prefix="${X11BINDIR}" \
		libdir="${X11USRLIBDIR}" \
		includedir="${X11INCDIR}" \
		hardcode_libdir_flag_spec="-Wl,-rpath,${X11USRLIBDIR}" \
		X_LIBS="${X11USRLIBDIR} -lX11" \
		C_FLAGS="${X11INCDIR} ${X11FLAGS.THREADLIB}" \
		VERSION="1.0.0"

xevie.pc:
	${CONFIG_SUBST} < ${XEVIEDIR}/xevie.pc.in > ${.TARGET}
.endif

.include <bsd.x11.mk>
.include <bsd.lib.mk>
