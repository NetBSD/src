#	$NetBSD: Makefile,v 1.5 2002/03/28 02:24:09 lukem Exp $

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.own.mk>
.include <bsd.kernobj.mk>

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk/ && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

CLEANFILES=	netbsd.tmp

.for kernel in INSTALL
KERNELS+=	netbsd.${kernel}.gz
CLEANFILES+=	netbsd.${kernel}.gz
netbsd.${kernel}.gz: .NOTMAIN ${KERNOBJDIR}/${kernel}/netbsd ${RAMDISK}
	cp ${KERNOBJDIR}/${kernel}/netbsd netbsd.tmp
	${MDSETIMAGE} -v netbsd.tmp ${RAMDISK}
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}
.endfor

all: ${KERNELS}

release:
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p ${KERNELS} ${RELEASEDIR}/binary/kernel

.include <bsd.prog.mk>
