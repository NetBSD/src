#	$NetBSD: Makefile,v 1.2 2000/06/12 12:39:20 shin Exp $

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

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
MINIROOTDIR = ${.CURDIR}/../miniroot
MINIROOTOBJ != cd ${MINIROOTDIR}; make print-objdir

release:
	rm -f netbsd netbsd.gz
	cp ${KERNOBJDIR}/RAMDISK/netbsd .
	mdsetimage netbsd ${MINIROOTOBJ}/miniroot.fs
	gzip -9 netbsd
	${IINST} netbsd.gz ${ITARGET}/.
	cd ${KERNSRCDIR}/arch/${MACHINE}/stand/pbsdboot; make
	${IINST} ${PBSDBOOT} ${ITARGET}/.
	${IINST} ${PBSDBOOT1} ${ITARGET}/.
	${.CURDIR}/../sets/makesums -t ${ITARGET} '*.gz' '*.exe'
.endif

.include <bsd.subdir.mk>
