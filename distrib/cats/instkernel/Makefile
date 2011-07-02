#	$NetBSD: Makefile,v 1.20 2011/07/02 17:37:28 tsutsui Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL ${RAMDISK} netbsd-INSTALL.aout \
			INSTALL.ABLE ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel

MDSET_NOSTRIP=		# defined

REMOVE_SECTIONS= \
	-R .ident -R .arm.atpcs -R .comment -R .note.netbsd.ident \
	-R .debug_abbrev -R .debug_info -R .debug_line -R .debug_str \
	-R .debug_frame -R .debug_loc -R .debug_pubnames \
	-R .debug_aranges -R .debug_pubtypes -R .ARM.attributes

MDSET_POST.netbsd-INSTALL.aout= \
    ${OBJCOPY} -O a.out-arm-netbsd \
	${REMOVE_SECTIONS} \
	${.TARGET} ${.TARGET}.tmp ;\
	mv ${.TARGET}.tmp ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
