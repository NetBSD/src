#	$NetBSD: Makefile,v 1.11 2002/05/02 18:03:02 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

realall: inst.fs.gz

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk.sysinst && ${PRINTOBJDIR}

inst.fs.gz: inst.fs
	gzip -9vc inst.fs > ${.TARGET}
CLEANFILES+=	inst.fs.gz

inst.fs: ${RAMDISKDIR}/ramdisk.fs
	cp ${RAMDISKDIR}/ramdisk.fs ${.TARGET}
CLEANFILES+=	inst.fs

${RAMDISKDIR}/ramdisk.fs:
	cd ${.CURDIR}/../ramdisk.sysinst ; ${MAKE}

release: check_RELEASEDIR .WAIT inst.fs inst.fs.gz
	${RELEASE_INSTALL} inst.fs inst.fs.gz ${RELEASEDIR}/installation/floppy

.include <bsd.prog.mk>
