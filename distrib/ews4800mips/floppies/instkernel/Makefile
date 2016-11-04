#	$NetBSD: Makefile,v 1.6.6.1 2016/11/04 14:42:38 pgoyette Exp $

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
	gunzip -c ${DESTDIR}/usr/mdec/boot_kernel.gz > ${BOOT_RAMDISK}.tmp
	${TOOL_MDSETIMAGE} -v ${BOOT_RAMDISK}.tmp netbsd-RAMDISK.gz
	${ELF2ECOFF} ${BOOT_RAMDISK}.tmp ${BOOT_RAMDISK}.coff
	${TOOL_GZIP_N} -9c ${BOOT_RAMDISK}.coff > ${.TARGET}

CLEANFILES+=	${BOOT_RAMDISK}.tmp ${BOOT_RAMDISK}.coff ${BOOT_RAMDISK}.gz

release::	check_RELEASEDIR .WAIT ${BOOT_RAMDISK}.gz
	${RELEASE_INSTALL} ${BOOT_RAMDISK}.gz \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/${BOOT_RELEASEDIR}

.include <bsd.prog.mk>
