#	$NetBSD: Makefile,v 1.2 1999/06/30 18:36:37 ross Exp $

SUBDIR=	instkernel floppy-GENERIC

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY}	\
		-o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE}

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release:
.for i in floppy diskimage instkernel tapeimage
	${IINST} README.files ${ITARGET}/$i/.
.endfor
	${IINST} instkernel/cd-hd-tape/obj/cdhdtape ${ITARGET}/diskimage/.
	${IINST} instkernel/fdset/obj/disk1of2 ${ITARGET}/floppy/.
	${IINST} instkernel/fdset/obj/disk2of2 ${ITARGET}/floppy/.
	${IINST} instkernel/instkernel/obj/netbsd.gz ${ITARGET}/instkernel/.
	${IINST} floppy-GENERIC/obj/GENERIC.fs ${ITARGET}/misc/.
.endif

.include <bsd.subdir.mk>
