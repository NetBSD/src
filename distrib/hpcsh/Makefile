#	$NetBSD: Makefile,v 1.4 2003/07/10 10:34:07 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

MINIROOTOBJ!=	cd ${.CURDIR}/../miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTOBJ}/miniroot.fs

MDSETTARGETS=		RAMDISK		${MINIROOT}	netbsd
MDSET_RELEASEDIR=	installation

HPCBOOTDIR=	${KERNSRCDIR}/arch/hpc/stand
HPCBOOT_SH3=	${HPCBOOTDIR}/binary/SH3/hpcboot.exe.uu
HPCBOOT_SH4=	${HPCBOOTDIR}/binary/SH4/hpcboot.exe.uu

release:: check_RELEASEDIR .WAIT ${HPCBOOT_SH3} ${HPCBOOT_SH4}
	(cd ${RELEASEDIR}/${MACHINE}/installation; rm -f hpcboot-sh3.exe; \
	    ${TOOL_UUDECODE} -p ${HPCBOOT_SH3} >hpcboot-sh3.exe ; \
	    chmod ${NONBINMODE} hpcboot-sh3.exe)
	(cd ${RELEASEDIR}/${MACHINE}/installation; rm -f hpcboot-sh4.exe; \
	    ${TOOL_UUDECODE} -p ${HPCBOOT_SH4} >hpcboot-sh4.exe ; \
	    chmod ${NONBINMODE} hpcboot-sh4.exe)
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation '*.gz' '*.exe'

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
