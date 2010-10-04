#	$NetBSD: Makefile,v 1.21 2010/10/04 13:45:28 tsutsui Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel

MDSET_SUFFIXES.-=	aout create-aout
create-aout= \
	${OBJCOPY} -O a.out-arm-netbsd \
	    -R .ident -R .arm.atpcs -R .comment -R .note.netbsd.ident \
	    -R .debug_abbrev -R .debug_info -R .debug_line -R .debug_str \
	    -R .debug_frame -R .debug_loc -R .debug_pubnames \
	    -R .debug_aranges -R .ARM.attributes \
	    ${.TARGET:R} ${.TARGET}

release::	check_RELEASEDIR .WAIT netbsd-INSTALL.aout.gz
	${RELEASE_INSTALL} netbsd-INSTALL.aout.gz \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/instkernel/netbsd.gz

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
