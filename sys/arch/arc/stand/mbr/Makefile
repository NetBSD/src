#	$NetBSD: Makefile,v 1.10 2004/06/30 03:26:26 jmc Exp $
#	$OpenBSD: Makefile,v 1.2 1997/08/01 12:32:10 pefo Exp $

UUDECODE_FILES	= mbr msdos5mb.gz

realinstall: ${UUDECODE_FILES}
	${INSTALL_FILE} -o ${BINOWN} -g ${BINGRP} -m 444 \
		${UUDECODE_FILES} ${DESTDIR}/usr/mdec

.include <bsd.files.mk>
.include <bsd.obj.mk>
.include <bsd.inc.mk>
