# $NetBSD: Makefile,v 1.5 2008/10/26 15:54:19 apb Exp $

CLEANFILES+=	atf-host-compile
CLEANFILES+=	atf-host-compile.tmp

.include <bsd.hostprog.mk>

realall: atf-host-compile
atf-host-compile: atf-host-compile.sh
	@echo "create atf-host-compile"; \
	if [ x${HOST_SH:C,^/.*,/,} != x/ ]; then \
		echo "HOST_SH must be an absolute path"; \
		false; \
	fi && \
	${TOOL_SED} -e s,__ATF_PKGDATADIR__,${SRCDIR}/atf-sh,g \
	            -e s,__ATF_SHELL__,${HOST_SH},g \
	            <${SRCDIR}/tools/atf-host-compile.sh \
	            >atf-host-compile.tmp && \
	chmod +x atf-host-compile.tmp && \
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
