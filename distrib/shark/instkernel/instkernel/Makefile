#	$NetBSD: Makefile,v 1.23 2011/07/10 04:37:56 tsutsui Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel

MDSET_SUFFIXES.-=	aout create-aout

create-aout= \
	${OBJCOPY} ${OBJCOPY_ELF2AOUT_FLAGS} ${.TARGET:R} ${.TARGET}

release::	check_RELEASEDIR .WAIT netbsd-INSTALL.aout.gz
	${RELEASE_INSTALL} netbsd-INSTALL.aout.gz \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/instkernel/netbsd.gz

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
