#	$NetBSD: Makefile,v 1.4 2001/04/22 02:04:22 shin Exp $

.include "../Makefile.inc"

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.kernobj.mk>

SUBDIR=

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY}	\
		-o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE}

PBSDBOOTDIR = ${KERNSRCDIR}/arch/${MACHINE}/stand/pbsdboot
PBSDBOOT = ${PBSDBOOTDIR}/pbsdboot.exe
PBSDBOOT1 = ${PBSDBOOTDIR}/pbsdboot1.exe

INSTKERNELS = TX3912 TX3922

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
MINIROOTDIR = ${.CURDIR}/../miniroot
MINIROOTOBJ != cd ${MINIROOTDIR}; ${MAKE} print-objdir

release:
	rm -f netbsd netbsd.gz
	cp ${KERNOBJDIR}/RAMDISK/netbsd .
	mdsetimage netbsd ${MINIROOTOBJ}/miniroot.fs
	gzip -9 netbsd
	${IINST} netbsd.gz ${ITARGET}/.
.for instkern in ${INSTKERNELS}
	rm -f netbsd netbsd.gz
	cp ${KERNOBJDIR}/INSTALL_${instkern}/netbsd .
	mdsetimage netbsd ${MINIROOTOBJ}/miniroot.fs
	gzip -9 netbsd
	${IINST} netbsd.gz ${ITARGET}/netbsd.${instkern}.gz
.endfor
	cd ${KERNSRCDIR}/arch/${MACHINE}/stand/pbsdboot; make
	${IINST} ${PBSDBOOT} ${ITARGET}/.
	${IINST} ${PBSDBOOT1} ${ITARGET}/.
	${.CURDIR}/../sets/makesums -t ${ITARGET} '*.gz' '*.exe'
.endif

.include <bsd.subdir.mk>
