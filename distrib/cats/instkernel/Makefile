#	$NetBSD: Makefile,v 1.21 2011/07/10 04:37:56 tsutsui Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL ${RAMDISK} netbsd-INSTALL.aout \
			INSTALL.ABLE ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel

MDSET_NOSTRIP=		# defined

MDSET_POST.netbsd-INSTALL.aout= \
    ${OBJCOPY} ${OBJCOPY_ELF2AOUT_FLAGS} ${.TARGET} ${.TARGET}.tmp ;\
	mv ${.TARGET}.tmp ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
