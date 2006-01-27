#       $NetBSD: Makefile,v 1.1.4.1 2006/01/27 22:43:39 tron Exp $
#

MISC_FILES=	runbootblock.README
UUDECODE_FILES=	device-streams.tar.gz \
		loadbsd loadbsd-2.14 loadbsd-2.16 \
		rdbinfo runbootblock xstreamtodev

.include <bsd.files.mk>
.include <bsd.hostprog.mk>

release: check_RELEASEDIR .WAIT ${MISC_FILES} ${UUDECODE_FILES}
	${_MKMSG_INSTALL} ${MISC_FILES}
	cd ${.CURDIR} && \
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${MISC_FILES} \
	    ${RELEASEDIR}/${MACHINE}/installation/misc
	${_MKMSG_INSTALL} ${UUDECODE_FILES}
	cd ${.OBJDIR} && \
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${UUDECODE_FILES} \
	    ${RELEASEDIR}/${MACHINE}/installation/misc

