#	$NetBSD: Makefile,v 1.19 2010/10/04 15:56:53 tsutsui Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL ${RAMDISK} netbsd-INSTALL.aout \
			INSTALL.ABLE ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel

MDSET_NOSTRIP=		# defined

MDSET_POST.netbsd-INSTALL.aout= \
    ${OBJCOPY} -O a.out-arm-netbsd \
	-R .debug_abbrev -R .debug_info -R .debug_line -R .debug_str \
	-R .debug_frame -R .debug_loc -R .debug_pubnames -R .debug_aranges \
	-R .ident -R .arm.atpcs -R .comment -R .note.netbsd.ident \
	-R .ARM.attributes \
	${.TARGET} ${.TARGET}.tmp ;\
	mv ${.TARGET}.tmp ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
