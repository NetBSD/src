#	$NetBSD: Makefile,v 1.26 2004/03/19 08:07:28 jmc Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

MINIROOTOBJ!=	cd ${.CURDIR}/../miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTOBJ}/miniroot.fs

MDSETTARGETS=		RAMDISK		${MINIROOT}	netbsd		\
			INSTALL_TX3912	${MINIROOT}	netbsd-TX3912
MDSET_RELEASEDIR=	installation

PBSDBOOTDIR=	${KERNSRCDIR}/arch/${MACHINE}/stand/pbsdboot
PBSDBOOT=	pbsdboot.exe
PBSDBOOT1=	pbsdboot1.exe
HPCBOOTDIR=	${KERNSRCDIR}/arch/hpc/stand/binary/MIPS
HPCBOOT=	hpcboot.exe

.PATH: ${PBSDBOOTDIR} ${HPCBOOTDIR}

UUDECODE_FILES=	${PBSDBOOT1} ${PBSDBOOT} ${HPCBOOT}
UUDECODE_FILES_RENAME_${HPCBOOT}= ${HPCBOOT}

.include "${DISTRIBDIR}/common/Makefile.mdset"

release:: check_RELEASEDIR .WAIT ${PBSDBOOT1} ${PBSDBOOT} ${HPCBOOT}
	cp pbsdboot1.exe ${RELEASEDIR}/${MACHINE}/installation; \
	chmod ${NONBINMODE} ${RELEASEDIR}/${MACHINE}/installation/pbsdboot1.exe
	cp pbsdboot.exe ${RELEASEDIR}/${MACHINE}/installation; \
	chmod ${NONBINMODE} ${RELEASEDIR}/${MACHINE}/installation/pbsdboot.exe
	cp hpcboot.exe ${RELEASEDIR}/${MACHINE}/installation; \
	chmod ${NONBINMODE} ${RELEASEDIR}/${MACHINE}/installation/hpcboot.exe
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation '*.gz' '*.exe'

.include <bsd.files.mk>
.include <bsd.prog.mk>
