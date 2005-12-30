#	$NetBSD: Makefile,v 1.2 2005/12/30 11:48:15 rjs Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

MINIROOTOBJ!=	cd ${.CURDIR}/../miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTOBJ}/miniroot.fs

MDSETTARGETS=		INSTALL_IPAQ	${MINIROOT}	netbsd-IPAQ
MDSET_RELEASEDIR=	installation

HPCBOOTDIR=	${KERNSRCDIR}/arch/hpc/stand/binary/ARM
HPCBOOT=	hpcboot.exe

.PATH: ${HPCBOOTDIR}

UUDECODE_FILES=	${HPCBOOT}
UUDECODE_FILES_RENAME_${HPCBOOT}= ${HPCBOOT}

.include "${DISTRIBDIR}/common/Makefile.mdset"

release:: check_RELEASEDIR .WAIT ${HPCBOOT}
	${INSTALL} -c -m ${NONBINMODE} \
		hpcboot.exe ${RELEASEDIR}/${MACHINE}/installation
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation '*.gz' '*.exe'

.include <bsd.files.mk>
.include <bsd.prog.mk>

