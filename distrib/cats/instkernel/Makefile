#	$NetBSD: Makefile,v 1.21.42.1 2019/01/18 08:48:31 pgoyette Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL ${RAMDISK} netbsd-INSTALL.aout \
			INSTALL.ABLE ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel

MDSET_NOSTRIP=		# defined

MDSET_POST.netbsd-INSTALL.aout= \
    ${ARM_ELF2AOUT} ${.TARGET} ${.TARGET}.tmp && \
	mv ${.TARGET}.tmp ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
