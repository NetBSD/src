# $NetBSD: Makefile,v 1.2.4.2 2008/01/09 01:59:58 matt Exp $

CLEANFILES+=	atf-host-compile
CLEANFILES+=	atf-host-compile.tmp

.include <bsd.hostprog.mk>

realall: atf-host-compile
atf-host-compile: atf-host-compile.sh
	@echo "create atf-host-compile"; \
	if [ -x ${HOST_SH} ]; then \
		H=${HOST_SH}; \
	elif [ -x /bin/${HOST_SH} ]; then \
		H=/bin/${HOST_SH}; \
	else \
		H=/usr/bin/${HOST_SH}; \
	fi; \
	${HOST_SED} -e s,__ATF_PKGDATADIR__,${SRCDIR}/subrs,g \
	            -e s,__ATF_SHELL__,$$H,g \
	            <${SRCDIR}/tools/atf-host-compile.sh \
	            >atf-host-compile.tmp; \
	chmod +x atf-host-compile.tmp; \
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
