#	$NetBSD: Makefile,v 1.3.4.1 2009/05/13 19:17:42 jym Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		RAMDISK ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

BOOT_RAMDISK=		boot-RAMDISK
BOOT_RELEASEDIR=	installation/boot

all realall:	${BOOT_RAMDISK}.gz

${BOOT_RAMDISK}.gz:	netbsd-RAMDISK.gz ${DESTDIR}/usr/mdec/boot_kernel.gz
	gunzip -c ${DESTDIR}/usr/mdec/boot_kernel.gz > ${BOOT_RAMDISK}.tmp
	${TOOL_MDSETIMAGE} ${BOOT_RAMDISK}.tmp netbsd-RAMDISK.gz
	${ELF2ECOFF} ${BOOT_RAMDISK}.tmp ${BOOT_RAMDISK}.coff
	gzip -9nc ${BOOT_RAMDISK}.coff > ${.TARGET}

CLEANFILES+=	${BOOT_RAMDISK}.tmp ${BOOT_RAMDISK}.coff ${BOOT_RAMDISK}.gz

release::	check_RELEASEDIR .WAIT ${BOOT_RAMDISK}.gz
	${RELEASE_INSTALL} ${BOOT_RAMDISK}.gz \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/${BOOT_RELEASEDIR}

.include <bsd.prog.mk>
