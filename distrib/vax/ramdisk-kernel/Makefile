#	$NetBSD: Makefile,v 1.4 2002/03/05 21:23:30 thorpej Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.own.mk>
.include <bsd.kernobj.mk>

MDEC=		${DESTDIR}/usr/mdec
STRIP?=		strip
MDSETIMAGE?=	${DESTDIR}/usr/sbin/mdsetimage

RAMDISK!=	cd ${.CURDIR}/../ramdisk/ && ${PRINTOBJDIR}

.for kernel in INSTALL
netbsd.${kernel}.gz: .NOTMAIN ${KERNOBJDIR}/${kernel}/netbsd ${RAMDISK}
	cp ${KERNOBJDIR}/${kernel}/netbsd netbsd.tmp
	${MDSETIMAGE} -v netbsd.tmp ${RAMDISK}/ramdisk.fs
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}
KERNELS+=	netbsd.${kernel}.gz
.endfor

all: ${KERNELS}

release:
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p ${KERNELS} ${RELEASEDIR}/binary/kernel

clean cleandir distclean:
	rm -f *.core ${KERNELS}

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
