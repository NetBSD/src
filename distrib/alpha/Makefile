#	$NetBSD: Makefile,v 1.9 2000/10/26 04:53:56 garbled Exp $

.include <bsd.own.mk>		# Use /etc/mk.conf.

SUBDIR=	instkernel #floppy-GENERIC

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY}	\
		-o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE}

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
CDHDTAPEOBJ != cd ${.CURDIR}/instkernel/cd-hd-tape; ${MAKE} print-objdir
FDSETOBJ != cd ${.CURDIR}/instkernel/fdset; ${MAKE} print-objdir
INSTKERNOBJ != cd ${.CURDIR}/instkernel/instkernel; ${MAKE} print-objdir
GENERICOBJ != cd ${.CURDIR}/floppy-GENERIC; ${MAKE} print-objdir

release:
.for i in floppy diskimage instkernel tapeimage
	${IINST} README.files ${ITARGET}/$i/.
.endfor
	${IINST} ${CDHDTAPEOBJ}/cdhdtape ${ITARGET}/diskimage/.
	${IINST} ${FDSETOBJ}/disk1of2 ${ITARGET}/floppy/.
	${IINST} ${FDSETOBJ}/disk2of2 ${ITARGET}/floppy/.
	${IINST} ${INSTKERNOBJ}/netbsd.gz ${ITARGET}/instkernel/.
	#${IINST} ${GENERICOBJ}/GENERIC.fs ${ITARGET}/misc/.
.endif

.include <bsd.subdir.mk>
