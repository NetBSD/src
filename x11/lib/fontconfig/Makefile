#	$NetBSD: Makefile,v 1.7 2004/03/11 16:13:30 rtr Exp $

.include <bsd.own.mk>

SUBDIR=	fontconfig src etc

FILES=		fontconfig.pc
FILESDIR=	${X11USRLIBDIR}/pkgconfig

SCRIPTS=	fontconfig-config

CLEANFILES+= ${FILES} ${SCRIPTS}
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

fontconfig-config:
	${CONFIG_SUBST} < ${FONTCONFIGDIR}/fontconfig-config.in > ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.prog.mk>	# for SCRIPTS
.include <bsd.subdir.mk>
