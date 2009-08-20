#	$NetBSD: Makefile,v 1.18 2009/08/20 14:43:06 skrll Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL ${RAMDISK} netbsd.aout-INSTALL \
			INSTALL.ABLE ${RAMDISK} netbsd.able-INSTALL
MDSET_RELEASEDIR=	binary/kernel

MDSET_NOSTRIP=		# defined

MDSET_POST.netbsd.aout-INSTALL= \
    ${OBJCOPY} -O a.out-arm-netbsd \
	-R .debug_abbrev -R .debug_info -R .debug_line -R .debug_str \
	-R .debug_frame -R .debug_loc -R .debug_pubnames -R .debug_aranges \
	-R .ident -R .arm.atpcs -R .comment -R .note.netbsd.ident \
	-R .ARM.attributes \
	${.TARGET} ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
