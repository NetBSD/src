#	$NetBSD: Makefile,v 1.8 2001/05/09 16:28:10 nsmrtks Exp $

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.depall.mk>
.include <bsd.prog.mk>
.MAIN:	all

realall: inst.fs.gz
all: realall

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk.sysinst ; ${MAKE} print-objdir

inst.fs.gz: inst.fs
	gzip -9vc inst.fs > ${.TARGET}
CLEANFILES+=	inst.fs.gz

inst.fs: ${RAMDISKDIR}/ramdisk.fs
	cp ${RAMDISKDIR}/ramdisk.fs ${.TARGET}
CLEANFILES+=	inst.fs

${RAMDISKDIR}/ramdisk.fs:
	cd ${.CURDIR}/../ramdisk.sysinst ; ${MAKE}

release:
	-mkdir -p ${RELEASEDIR}/installation/floppy
	cp -p inst.fs inst.fs.gz ${RELEASEDIR}/installation/floppy
