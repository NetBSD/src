#       $NetBSD: Makefile,v 1.6 2023/03/23 13:56:24 abs Exp $
#

#include <bsd.hostinit.mk>

MISC_FILES=	runbootblock.README device-streams.README.md
UUDECODE_FILES=	device-streams-2.2.1-src.tar.gz device-streams-2.2.1.lha \
		loadbsd rdbinfo runbootblock xdevtostream xstreamtodev

.include <bsd.files.mk>
.include <bsd.hostprog.mk>

release: check_RELEASEDIR .WAIT ${MISC_FILES} ${UUDECODE_FILES}
	${_MKMSG_INSTALL} ${MISC_FILES}
	cd ${.CURDIR} && \
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${MISC_FILES} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc
	${_MKMSG_INSTALL} ${UUDECODE_FILES}
	cd ${.OBJDIR} && \
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${UUDECODE_FILES} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc

