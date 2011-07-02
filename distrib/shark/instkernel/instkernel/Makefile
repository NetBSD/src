#	$NetBSD: Makefile,v 1.22 2011/07/02 17:37:28 tsutsui Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel

MDSET_SUFFIXES.-=	aout create-aout

REMOVE_SECTIONS= \
	-R .ident -R .arm.atpcs -R .comment -R .note.netbsd.ident \
	-R .debug_abbrev -R .debug_info -R .debug_line -R .debug_str \
	-R .debug_frame -R .debug_loc -R .debug_pubnames \
	-R .debug_aranges -R .debug_pubtypes -R .ARM.attributes

create-aout= \
	${OBJCOPY} -O a.out-arm-netbsd \
	    ${REMOVE_SECTIONS} \
	    ${.TARGET:R} ${.TARGET}

release::	check_RELEASEDIR .WAIT netbsd-INSTALL.aout.gz
	${RELEASE_INSTALL} netbsd-INSTALL.aout.gz \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/instkernel/netbsd.gz

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
