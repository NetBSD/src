#	$NetBSD: Makefile,v 1.1.2.1 2004/11/15 09:12:46 rtr Exp $

.include <bsd.own.mk>

LIB=	Xcomposite

XCOMPOSITEDIR=	${X11SRCDIR.xc}/lib/${LIB}
.PATH:		${XCOMPOSITEDIR}
SRCS=		Xcomposite.c

INCS=	Xcomposite.h
INCSDIR=${X11INCDIR}/X11/extensions

FILES=			xcomposite.pc
FILESDIR_xcomposite.pc=	${X11USRLIBDIR}/pkgconfig

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
		VERSION="1.0.0"

xcomposite.pc:
	${CONFIG_SUBST} < ${XCOMPOSITEDIR}/xcomposite.pc.in > ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
