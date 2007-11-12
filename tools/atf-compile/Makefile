# $NetBSD: Makefile,v 1.1 2007/11/12 14:54:43 jmmv Exp $

CLEANFILES+=	atf-host-compile
CLEANFILES+=	atf-host-compile.tmp

.include <bsd.hostprog.mk>

realall: atf-host-compile
atf-host-compile: atf-host-compile.sh
	${HOST_SED} -e 's,__ATF_PKGDATADIR__,${SRCDIR}/subrs,g' \
	            -e 's,__ATF_SHELL__,${HOST_SH},g' \
	            <${SRCDIR}/tools/atf-host-compile.sh \
	            >atf-host-compile.tmp
	chmod +x atf-host-compile.tmp
	mv atf-host-compile.tmp atf-host-compile

# Install rule.
realinstall: install.host
install.host: ${TOOLDIR}/bin/${_TOOL_PREFIX}atf-compile
${TOOLDIR}/bin/${_TOOL_PREFIX}atf-compile:: atf-host-compile
	${_MKTARGET_INSTALL}
	mkdir -p ${TOOLDIR}/bin
	${HOST_INSTALL_FILE} -m ${BINMODE} atf-host-compile ${.TARGET}

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/tools
