#	$NetBSD: Makefile,v 1.6.92.1 2011/01/10 00:37:34 jym Exp $

MISC_FILES=	rawrite.c
UUDECODE_FILES=	pfdisk.doc pfdisk.exe pfdisktc.zip \
		rawr32.exe \
		rawrite.doc rawrite.exe

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
