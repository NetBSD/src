#	$NetBSD: Makefile,v 1.12 2002/05/07 14:08:33 isaki Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

realall: inst.fs

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk.sysinst && ${MAKE} print-objdir

inst.fs: ${RAMDISKDIR}/ramdisk.fs
	cp ${RAMDISKDIR}/ramdisk.fs inst.fs

CLEANFILES+=	inst.fs inst.fs.gz

${RAMDISKDIR}/ramdisk.fs:
	cd ${.CURDIR}/../ramdisk.sysinst ; ${MAKE}

release: check_RELEASEDIR .WAIT inst.fs
	${RELEASE_INSTALL} inst.fs ${RELEASEDIR}/installation/floppy
	gzip -9v <${RELEASEDIR}/installation/floppy/inst.fs >${RELEASEDIR}/installation/floppy/inst.fs.gz

.include <bsd.prog.mk>
