#	$NetBSD: Makefile,v 1.3 2004/01/06 06:15:45 rtr Exp $

.include <bsd.own.mk>

SUBDIR=	fontconfig src etc

FILES=		fontconfig.pc
FILESDIR=	${X11USRLIBDIR}/pkgconfig

SCRIPTS=	fontconfig-config

.PHONY:		fontconfig.pc fontconfig-config
all dependall:	fontconfig.pc fontconfig-config

# XXX this should be ${SHELL} but I'm not getting joy w/ it.
FONTCONFIGDIR=	${X11SRCDIR.xc}/lib/fontconfig
CONFIG_SUBST=	sh ${FONTCONFIGDIR}/config/config-subst \
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
