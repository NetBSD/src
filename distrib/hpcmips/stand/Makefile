#	$NetBSD: Makefile,v 1.1.6.3 2008/06/29 08:25:12 mjf Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

PBSDBOOTDIR=	${KERNSRCDIR}/arch/${MACHINE}/stand/pbsdboot
PBSDBOOT=	pbsdboot.exe
PBSDBOOT1=	pbsdboot1.exe
HPCBOOTDIR=	${KERNSRCDIR}/arch/hpc/stand/binary/MIPS
HPCBOOT=	hpcboot.exe

.PATH: ${PBSDBOOTDIR} ${HPCBOOTDIR}

UUDECODE_FILES=	${PBSDBOOT1} ${PBSDBOOT} ${HPCBOOT}
UUDECODE_FILES_RENAME_${HPCBOOT}= ${HPCBOOT}

release: ${UUDECODE_FILES}
	${INSTALL} -c -m ${NONBINMODE} \
		${PBSDBOOT1} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation
	${INSTALL} -c -m ${NONBINMODE} \
		${PBSDBOOT} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation
	${INSTALL} -c -m ${NONBINMODE} \
		${HPCBOOT} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation

.include <bsd.files.mk>
.include <bsd.obj.mk>
