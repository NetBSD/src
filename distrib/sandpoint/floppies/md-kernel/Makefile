#	$NetBSD: Makefile,v 1.1 2001/12/07 05:04:59 briggs Exp $

TOP= ${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.own.mk>
.include <bsd.kernobj.mk>

STRIP?= strip
MDSETIMAGE?=	mdsetimage

KERNELS= netbsd.INSTALL.gz
CLEANFILES+= ${KERNELS} netbsd

RAMDISKDIR!= cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK= ${RAMDISKDIR}/ramdisk.fs

all dependall: ${KERNELS}

netbsd.INSTALL.gz: ${KERNOBJDIR}/INSTALL/netbsd ${RAMDISK}
	cp ${KERNOBJDIR}/INSTALL/netbsd .
	${MDSETIMAGE} -v netbsd ${RAMDISK}
	${STRIP} netbsd
	gzip -9 netbsd
	mv netbsd.gz ${.TARGET}

release: ${KERNELS}
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp ${KERNELS} ${RELEASEDIR}/binary/kernel

clean cleandir distclean:
	rm -f *.core ${CLEANFILES}

includes:

.include <bsd.obj.mk>
