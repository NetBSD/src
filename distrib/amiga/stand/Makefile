#       $NetBSD: Makefile,v 1.4.14.1 2018/05/02 07:19:59 pgoyette Exp $
#

#include <bsd.hostinit.mk>

MISC_FILES=	runbootblock.README
UUDECODE_FILES=	device-streams.tar.gz \
		loadbsd rdbinfo runbootblock xstreamtodev

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

