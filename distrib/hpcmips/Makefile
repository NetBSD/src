#	$NetBSD: Makefile,v 1.13 2002/04/11 13:50:06 shin Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

.include <bsd.kernobj.mk>

ITARGET=	${RELEASEDIR}/installation

PBSDBOOTDIR=	${KERNSRCDIR}/arch/${MACHINE}/stand/pbsdboot
PBSDBOOT=	${PBSDBOOTDIR}/pbsdboot.uu
PBSDBOOT1=	${PBSDBOOTDIR}/pbsdboot1.uu
HPCBOOTDIR=	${KERNSRCDIR}/arch/hpc/stand
HPCBOOT=	${HPCBOOTDIR}/binary/MIPS/hpcboot.exe.uu

MINIROOTOBJ!=	cd ${.CURDIR}/../miniroot && ${PRINTOBJDIR}

#	TARGETS is a list of kernel directories and generated images:
#		Kernel			Image
#
TARGETS=	RAMDISK			netbsd		\
		INSTALL_TX3912		netbsd.TX3912

.for K I in ${TARGETS}					# {
IMAGES+=	${I}.gz
${I}.gz: .NOTMAIN ${KERNOBJDIR}/${K}/netbsd ${MINIROOTOBJ}/miniroot.fs
	cp ${KERNOBJDIR}/${K}/netbsd netbsd.tmp
	${MDSETIMAGE} netbsd.tmp ${MINIROOTOBJ}/miniroot.fs
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}
.endfor							# }

CLEANFILES+=	netbsd.tmp netbsd.tmp.gz ${IMAGES}

#	do the work
#
realall: ${IMAGES}

release: check_RELEASEDIR .WAIT ${IMAGES}
.for img in ${IMAGES}
	${RELINSTALL} ${img} ${ITARGET}/.
.endfor
	(cd ${ITARGET}; ${UUDECODE} ${PBSDBOOT1}; \
	    chmod ${NONBINMODE} pbsdboot1.exe)
	(cd ${ITARGET}; ${UUDECODE} ${PBSDBOOT}; \
	    chmod ${NONBINMODE} pbsdboot.exe)
	(cd ${ITARGET}; rm -f hpcboot.exe; \
	    ${UUDECODE} -p ${HPCBOOT} >hpcboot.exe ; \
	    chmod ${NONBINMODE} hpcboot.exe)
	${MAKESUMS} -t ${ITARGET} '*.gz' '*.exe'

.include <bsd.prog.mk>
