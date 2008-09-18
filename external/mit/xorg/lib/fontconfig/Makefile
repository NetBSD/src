#	$NetBSD: Makefile,v 1.2.2.2 2008/09/18 05:18:35 wrstuden Exp $

.include <bsd.own.mk>

SUBDIR=	fontconfig src etc

.if 0
FILES=		fontconfig.pc
FILESDIR=	${X11USRLIBDIR}/pkgconfig

CLEANFILES+= ${FILES}
realall: ${FILES}

FONTCONFIGDIR=	${X11SRCDIR.fontconfig}
CONFIG_SUBST=	${HOST_SH} ${FONTCONFIGDIR}/config/config-subst \
		prefix="${X11ROOTDIR}" \
		exec_prefix="${X11BINDIR}" \
		libdir="${X11USRLIBDIR}" \
		includedir="${X11INCDIR}" \
		hardcode_libdir_flag_spec="-Wl,-rpath,${X11USRLIBDIR}" \
		PACKAGE_VERSION="1.0.1"

fontconfig.pc:
	${CONFIG_SUBST} < ${FONTCONFIGDIR}/fontconfig.pc.in > ${.TARGET}
.endif

.include <bsd.x11.mk>
.include <bsd.prog.mk>	# for SCRIPTS
.include <bsd.subdir.mk>
