#	$NetBSD: Makefile.inc,v 1.3 2002/03/05 21:23:30 thorpej Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.own.mk>
.include <bsd.subdir.mk>

IMAGE?=		boot.fs
KERNDIR?=	cd ${.CURDIR}/../kernel-ramdisk && ${PRINTOBJDIR}
KERN?=		${KERNDIR}/netbsd.INSTALL.gz

MDEC?=		${DESTDIR}/usr/mdec
MKBOOTIMAGE?=	${MDEC}/mkbootimage
BOOTLOADER?=	${MDEC}/boot_com0
MDSETIMAGE?=	mdsetimage

realall: ${IMAGE}

${IMAGE}: ${KERN}
	${MKBOOTIMAGE} ${BOOTLOADER} ${.TARGET} ${KERN}

.ifndef RELEASEDIR
release:
	@echo set RELEASEDIR first!
	@false
.else
release:
	-mkdir -p ${RELEASEDIR}/installation/floppy
	cp -p ${IMAGE} ${RELEASEDIR}/installation/floppy
	gzip -9c ${IMAGE} > ${RELEASEDIR}/installation/floppy/${IMAGE}.gz
.endif

clean cleandir distclean:
	rm -f *.core ${IMAGE} ${CLEANFILES}

.include <bsd.obj.mk>
