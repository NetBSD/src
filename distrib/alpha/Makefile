#	$NetBSD: Makefile,v 1.11 2001/11/14 00:44:13 tv Exp $

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
CDHDTAPEOBJ != cd ${.CURDIR}/instkernel/cd-hd-tape && ${PRINTOBJDIR}
FDSETOBJ != cd ${.CURDIR}/instkernel/fdset && ${PRINTOBJDIR}
INSTKERNOBJ != cd ${.CURDIR}/instkernel/instkernel && ${PRINTOBJDIR}
GENERICOBJ != cd ${.CURDIR}/floppy-GENERIC && ${PRINTOBJDIR}

release:
.for i in floppy diskimage instkernel tapeimage
	${IINST} ${.CURDIR}/README.files ${ITARGET}/$i/.
.endfor
	${IINST} ${CDHDTAPEOBJ}/cdhdtape ${ITARGET}/diskimage/.
	${IINST} ${FDSETOBJ}/disk1of2 ${ITARGET}/floppy/.
	${IINST} ${FDSETOBJ}/disk2of2 ${ITARGET}/floppy/.
	${IINST} ${INSTKERNOBJ}/netbsd.gz ${ITARGET}/instkernel/.
	#${IINST} ${GENERICOBJ}/GENERIC.fs ${ITARGET}/misc/.
.endif

.include <bsd.subdir.mk>
