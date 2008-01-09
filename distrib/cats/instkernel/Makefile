#	$NetBSD: Makefile,v 1.14.10.1 2008/01/09 01:27:51 matt Exp $

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
	-R .ident -R .arm.atpcs -R .comment -R .note.netbsd.ident \
	${.TARGET} ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
