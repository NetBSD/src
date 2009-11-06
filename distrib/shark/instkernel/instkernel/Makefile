#	$NetBSD: Makefile,v 1.19 2009/11/06 12:31:44 is Exp $

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

.include "${DISTRIBDIR}/common/Makefile.mdset"

.PHONY: release-aout-instkernel
release-aout-instkernel:
	${RELEASE_INSTALL} netbsd-INSTALL.aout.gz \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/instkernel/netbsd.gz

.include <bsd.prog.mk>
