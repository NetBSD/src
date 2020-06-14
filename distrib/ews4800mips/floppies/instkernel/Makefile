#	$NetBSD: Makefile,v 1.9 2020/06/14 05:10:33 tsutsui Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		RAMDISK ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel
MDSET_STRIPSECTIONS=	-R .MIPS.abiflags -R .gnu.attributes -R .pdr -R .reginfo

.include "${DISTRIBDIR}/common/Makefile.mdset"

BOOT_RAMDISK=		boot-RAMDISK
BOOT_RELEASEDIR=	installation/boot

all realall:	${BOOT_RAMDISK}.gz

${BOOT_RAMDISK}.gz:	netbsd-RAMDISK.gz ${DESTDIR}/usr/mdec/boot_kernel.gz
	@echo "Creating a bootloader binary with embedded RAMDISK kernel"
	gunzip -c ${DESTDIR}/usr/mdec/boot_kernel.gz > ${BOOT_RAMDISK}.tmp
	${TOOL_MDSETIMAGE} -v ${BOOT_RAMDISK}.tmp netbsd-RAMDISK.gz
	${MIPS_ELF2ECOFF} ${BOOT_RAMDISK}.tmp ${BOOT_RAMDISK}.coff
	${TOOL_GZIP_N} -9c ${BOOT_RAMDISK}.coff > ${.TARGET}

CLEANFILES+=	${BOOT_RAMDISK}.tmp ${BOOT_RAMDISK}.coff ${BOOT_RAMDISK}.gz

release::	check_RELEASEDIR .WAIT ${BOOT_RAMDISK}.gz
	${RELEASE_INSTALL} ${BOOT_RAMDISK}.gz \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/${BOOT_RELEASEDIR}

.include <bsd.prog.mk>
