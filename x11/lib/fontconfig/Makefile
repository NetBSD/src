#	$NetBSD: Makefile,v 1.7.2.1 2004/04/29 05:46:43 jmc Exp $

.include <bsd.own.mk>

SUBDIR=	fontconfig src etc

FILES=		fontconfig.pc
FILESDIR=	${X11USRLIBDIR}/pkgconfig

CLEANFILES+= ${FILES}
realall: ${FILES}

FONTCONFIGDIR=	${X11SRCDIR.xc}/extras/fontconfig
CONFIG_SUBST=	${HOST_SH} ${FONTCONFIGDIR}/config/config-subst \
		prefix="${X11ROOTDIR}" \
		exec_prefix="${X11BINDIR}" \
		libdir="${X11USRLIBDIR}" \
		includedir="${X11INCDIR}" \
		hardcode_libdir_flag_spec="-Wl,-rpath,${X11USRLIBDIR}" \
		PACKAGE_VERSION="1.0.1"

fontconfig.pc:
	${CONFIG_SUBST} < ${FONTCONFIGDIR}/fontconfig.pc.in > ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.prog.mk>	# for SCRIPTS
.include <bsd.subdir.mk>
