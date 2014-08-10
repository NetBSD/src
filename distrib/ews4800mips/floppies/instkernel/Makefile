#	$NetBSD: Makefile,v 1.5.2.1 2014/08/10 06:48:04 tls Exp $

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
	${TOOL_GZIP_N} -9c ${BOOT_RAMDISK}.coff > ${.TARGET}

CLEANFILES+=	${BOOT_RAMDISK}.tmp ${BOOT_RAMDISK}.coff ${BOOT_RAMDISK}.gz

release::	check_RELEASEDIR .WAIT ${BOOT_RAMDISK}.gz
	${RELEASE_INSTALL} ${BOOT_RAMDISK}.gz \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/${BOOT_RELEASEDIR}

.include <bsd.prog.mk>
