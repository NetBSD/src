#	$NetBSD: Makefile,v 1.7.58.2 2018/07/28 04:37:35 pgoyette Exp $

.include <bsd.hostinit.mk>

MISC_FILES=	rawrite.c
UUDECODE_FILES=	pfdisk.doc pfdisk.exe pfdisktc.zip \
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
