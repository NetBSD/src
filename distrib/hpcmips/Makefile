#	$NetBSD: Makefile,v 1.10 2002/03/23 03:21:23 shin Exp $

.include "../Makefile.inc"

.include <bsd.obj.mk>
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

SUBDIR=

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}

PBSDBOOTDIR = ${KERNSRCDIR}/arch/${MACHINE}/stand/pbsdboot
PBSDBOOT = ${PBSDBOOTDIR}/pbsdboot.exe
PBSDBOOT1 = ${PBSDBOOTDIR}/pbsdboot1.exe
HPCBOOTDIR = ${KERNSRCDIR}/arch/hpc/stand
HPCBOOT = ${HPCBOOTDIR}/binary/MIPS/hpcboot.exe
MDSETIMAGE?=	mdsetimage

INSTKERNELS = TX3912

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
MINIROOTDIR = ${.CURDIR}/../miniroot
MINIROOTOBJ != cd ${MINIROOTDIR} && ${PRINTOBJDIR}

release:
	rm -f netbsd netbsd.gz
	cp ${KERNOBJDIR}/RAMDISK/netbsd .
	${MDSETIMAGE} netbsd ${MINIROOTOBJ}/miniroot.fs
	gzip -9 netbsd
	${IINST} netbsd.gz ${ITARGET}/.
.for instkern in ${INSTKERNELS}
	rm -f netbsd netbsd.gz
	cp ${KERNOBJDIR}/INSTALL_${instkern}/netbsd .
	${MDSETIMAGE} netbsd ${MINIROOTOBJ}/miniroot.fs
	gzip -9 netbsd
	${IINST} netbsd.gz ${ITARGET}/netbsd.${instkern}.gz
.endfor
	cd ${KERNSRCDIR}/arch/${MACHINE}/stand/pbsdboot; make
	${IINST} ${PBSDBOOT} ${ITARGET}/.
	${IINST} ${PBSDBOOT1} ${ITARGET}/.
	cd ${KERNSRCDIR}/arch/hpc/stand; make
	${IINST} ${HPCBOOT} ${ITARGET}/.
	${.CURDIR}/../sets/makesums -t ${ITARGET} '*.gz' '*.exe'
.endif

.include <bsd.subdir.mk>
