#	$NetBSD: Makefile,v 1.25 2003/07/10 10:34:06 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

MINIROOTOBJ!=	cd ${.CURDIR}/../miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTOBJ}/miniroot.fs

MDSETTARGETS=		RAMDISK		${MINIROOT}	netbsd		\
			INSTALL_TX3912	${MINIROOT}	netbsd-TX3912
MDSET_RELEASEDIR=	installation

PBSDBOOTDIR=	${KERNSRCDIR}/arch/${MACHINE}/stand/pbsdboot
PBSDBOOT=	${PBSDBOOTDIR}/pbsdboot.uu
PBSDBOOT1=	${PBSDBOOTDIR}/pbsdboot1.uu
HPCBOOTDIR=	${KERNSRCDIR}/arch/hpc/stand
HPCBOOT=	${HPCBOOTDIR}/binary/MIPS/hpcboot.exe.uu

.include "${DISTRIBDIR}/common/Makefile.mdset"

release:: check_RELEASEDIR .WAIT ${PBSDBOOT1} ${PBSDBOOT} ${HPCBOOT}
	(cd ${RELEASEDIR}/${MACHINE}/installation; ${TOOL_UUDECODE} ${PBSDBOOT1}; \
	    chmod ${NONBINMODE} pbsdboot1.exe)
	(cd ${RELEASEDIR}/${MACHINE}/installation; ${TOOL_UUDECODE} ${PBSDBOOT}; \
	    chmod ${NONBINMODE} pbsdboot.exe)
	(cd ${RELEASEDIR}/${MACHINE}/installation; rm -f hpcboot.exe; \
	    ${TOOL_UUDECODE} -p ${HPCBOOT} >hpcboot.exe ; \
	    chmod ${NONBINMODE} hpcboot.exe)
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation '*.gz' '*.exe'

.include <bsd.prog.mk>
