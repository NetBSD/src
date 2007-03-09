#	$NetBSD: Makefile,v 1.5.16.1 2007/03/09 06:59:23 riz Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		cdroms

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
.include <bsd.kernobj.mk>

MINIROOTOBJ!=	cd ${.CURDIR}/../miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTOBJ}/miniroot.fs

MDSETTARGETS=		RAMDISK		${MINIROOT}	netbsd
MDSET_RELEASEDIR=	installation

HPCBOOTDIR=	${KERNSRCDIR}/arch/hpc/stand
HPCBOOT_SH3=	${HPCBOOTDIR}/binary/SH3
HPCBOOT_SH4=	${HPCBOOTDIR}/binary/SH4

UUDECODE_FILES= hpcboot-sh3.exe hpcboot-sh4.exe
UUDECODE_FILES_RENAME_hpcboot-sh3.exe= hpcboot-sh3.exe
UUDECODE_FILES_RENAME_hpcboot-sh4.exe= hpcboot-sh4.exe

CLEANFILES+= hpcboot-sh3.exe.uue hpcboot-sh4.exe.uue

hpcboot-sh3.exe.uue:
	cp ${HPCBOOT_SH3}/hpcboot.exe.uue hpcboot-sh3.exe.uue
hpcboot-sh4.exe.uue:
	cp ${HPCBOOT_SH4}/hpcboot.exe.uue hpcboot-sh4.exe.uue

release:: check_RELEASEDIR .WAIT hpcboot-sh3.exe hpcboot-sh4.exe
	${MAKEDIRTARGET} cdroms release
	cp hpcboot-sh3.exe ${RELEASEDIR}/${MACHINE}/installation; \
	chmod ${NONBINMODE} ${RELEASEDIR}/${MACHINE}/installation/hpcboot-sh3.exe
	cp hpcboot-sh4.exe ${RELEASEDIR}/${MACHINE}/installation; \
	chmod ${NONBINMODE} ${RELEASEDIR}/${MACHINE}/installation/hpcboot-sh4.exe
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation '*.gz' '*.exe'

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.files.mk>
.include <bsd.prog.mk>
