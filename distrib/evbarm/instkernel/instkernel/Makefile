#	$NetBSD: Makefile,v 1.2 2002/04/13 17:29:27 thorpej Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	TEAMASA_NPWR_INSTALL ${RAMDISK} -

MDEXTRA_SUFFIXES.TEAMASA_NPWR_INSTALL= srec bin
netbsd.TEAMASA_NPWR_INSTALL.post= \
	${OBJCOPY} -O srec netbsd.tmp netbsd.srec ; \
	gzip -9f netbsd.srec ; \
	${OBJCOPY} -O binary netbsd.tmp netbsd.bin ; \
	gzip -9f netbsd.bin

MDSETDIR=	installation/instkernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
