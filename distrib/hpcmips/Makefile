#	$NetBSD: Makefile,v 1.15 2002/04/12 05:33:59 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

.include <bsd.kernobj.mk>

MINIROOTOBJ!=	cd ${.CURDIR}/../miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTOBJ}/miniroot.fs
TARGETS=	RAMDISK		${MINIROOT}	\
		INSTALL_TX3912	${MINIROOT}

ITARGET=	${RELEASEDIR}/installation
PBSDBOOTDIR=	${KERNSRCDIR}/arch/${MACHINE}/stand/pbsdboot
PBSDBOOT=	${PBSDBOOTDIR}/pbsdboot.uu
PBSDBOOT1=	${PBSDBOOTDIR}/pbsdboot1.uu
HPCBOOTDIR=	${KERNSRCDIR}/arch/hpc/stand
HPCBOOT=	${HPCBOOTDIR}/binary/MIPS/hpcboot.exe.uu

release: check_RELEASEDIR .WAIT ${KERNELS}
	-mkdir -p ${ITARGET}
	${RELINSTALL} ${KERNELS} ${KERNELSYMS} ${ITARGET}
	(cd ${ITARGET}; ${UUDECODE} ${PBSDBOOT1}; \
	    chmod ${NONBINMODE} pbsdboot1.exe)
	(cd ${ITARGET}; ${UUDECODE} ${PBSDBOOT}; \
	    chmod ${NONBINMODE} pbsdboot.exe)
	(cd ${ITARGET}; rm -f hpcboot.exe; \
	    ${UUDECODE} -p ${HPCBOOT} >hpcboot.exe ; \
	    chmod ${NONBINMODE} hpcboot.exe)
	${MAKESUMS} -t ${ITARGET} '*.gz' '*.exe'

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
