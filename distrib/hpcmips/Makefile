#	$NetBSD: Makefile,v 1.20 2002/05/07 03:34:16 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

MINIROOTOBJ!=	cd ${.CURDIR}/../miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTOBJ}/miniroot.fs
MDSETTARGETS=	RAMDISK		${MINIROOT}	netbsd		\
		INSTALL_TX3912	${MINIROOT}	netbsd.TX3912

MDSET_RELEASEDIR=	installation

PBSDBOOTDIR=	${KERNSRCDIR}/arch/${MACHINE}/stand/pbsdboot
PBSDBOOT=	${PBSDBOOTDIR}/pbsdboot.uu
PBSDBOOT1=	${PBSDBOOTDIR}/pbsdboot1.uu
HPCBOOTDIR=	${KERNSRCDIR}/arch/hpc/stand
HPCBOOT=	${HPCBOOTDIR}/binary/MIPS/hpcboot.exe.uu

release:: check_RELEASEDIR .WAIT ${PBSDBOOT1} ${PBSDBOOT} ${HPCBOOT}
	(cd ${RELEASEDIR}/installation; ${UUDECODE} ${PBSDBOOT1}; \
	    chmod ${NONBINMODE} pbsdboot1.exe)
	(cd ${RELEASEDIR}/installation; ${UUDECODE} ${PBSDBOOT}; \
	    chmod ${NONBINMODE} pbsdboot.exe)
	(cd ${RELEASEDIR}/installation; rm -f hpcboot.exe; \
	    ${UUDECODE} -p ${HPCBOOT} >hpcboot.exe ; \
	    chmod ${NONBINMODE} hpcboot.exe)
	${MAKESUMS} -t ${RELEASEDIR}/installation '*.gz' '*.exe'

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
