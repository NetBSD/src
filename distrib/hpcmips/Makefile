#	$NetBSD: Makefile,v 1.11 2002/04/10 05:00:47 shin Exp $

.include "../Makefile.inc"

.include <bsd.obj.mk>
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

SUBDIR=

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}

PBSDBOOTDIR = ${KERNSRCDIR}/arch/${MACHINE}/stand/pbsdboot
PBSDBOOT = ${PBSDBOOTDIR}/pbsdboot.uu
PBSDBOOT1 = ${PBSDBOOTDIR}/pbsdboot1.uu
HPCBOOTDIR = ${KERNSRCDIR}/arch/hpc/stand
HPCBOOT = ${HPCBOOTDIR}/binary/MIPS/hpcboot.exe.uu
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
	(cd ${ITARGET}; ${UUDECODE} ${PBSDBOOT1}; chmod ${NONBINMODE} pbsdboot1.exe)
	(cd ${ITARGET}; ${UUDECODE} ${PBSDBOOT}; chmod ${NONBINMODE} pbsdboot.exe)
	(cd ${ITARGET}; rm -f hpcboot.exe; ${UUDECODE} -p ${HPCBOOT} >hpcboot.exe ; chmod ${NONBINMODE} hpcboot.exe)
	${.CURDIR}/../sets/makesums -t ${ITARGET} '*.gz' '*.exe'
.endif

.include <bsd.subdir.mk>
