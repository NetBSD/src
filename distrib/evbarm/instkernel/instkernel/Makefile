#	$NetBSD: Makefile,v 1.1 2002/04/13 01:46:31 thorpej Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	TEAMASA_NPWR_INSTALL ${RAMDISK} -

MDSETDIR=	installation/instkernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
