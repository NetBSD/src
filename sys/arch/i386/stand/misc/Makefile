#	$NetBSD: Makefile,v 1.3 2004/04/02 08:54:04 lukem Exp $

MISC_FILES=	rawrite.c
UUDECODE_FILES=	pfdisk.doc pfdisk.exe pfdisktc.zip \
		rawr32.zip rawr32src.zip \
		rawrite.doc rawrite.exe

.include <bsd.files.mk>
.include <bsd.hostprog.mk>

release: check_RELEASEDIR .WAIT ${MISC_FILES} ${UUDECODE_FILES}
	${_MKMSG_INSTALL} ${MISC_FILES}
	cd ${.CURDIR} && \
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${MISC_FILES} \
	    ${RELEASEDIR}/${MACHINE}/installation/misc
	${_MKMSG_INSTALL} ${UUDECODE_FILES}
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${UUDECODE_FILES} \
	    ${RELEASEDIR}/${MACHINE}/installation/misc
