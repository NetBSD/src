#	$NetBSD: Makefile,v 1.4 2004/01/08 04:45:12 rtr Exp $

.include <bsd.own.mk>

SUBDIR=	fontconfig src etc

FILES=		fontconfig.pc
FILESDIR=	${X11USRLIBDIR}/pkgconfig

SCRIPTS=	fontconfig-config

.PHONY:		fontconfig.pc fontconfig-config
all dependall:	fontconfig.pc fontconfig-config

FONTCONFIGDIR=	${X11SRCDIR.xc}/lib/fontconfig
CONFIG_SUBST=	${HOST_SH} ${FONTCONFIGDIR}/config/config-subst \
		prefix="${X11ROOTDIR}" \
		exec_prefix="${X11BINDIR}" \
		libdir="${X11USRLIBDIR}" \
		includedir="${X11INCDIR}" \
		PACKAGE_VERSION="1.0.1"

fontconfig.pc:
	${CONFIG_SUBST} < ${FONTCONFIGDIR}/fontconfig.pc.in > ${.TARGET}

fontconfig-config:
	${CONFIG_SUBST} < ${FONTCONFIGDIR}/fontconfig-config.in > ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.prog.mk>	# for SCRIPTS
.include <bsd.subdir.mk>
