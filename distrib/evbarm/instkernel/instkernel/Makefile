#	$NetBSD: Makefile,v 1.4 2002/04/26 15:50:30 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	TEAMASA_NPWR_INSTALL ${RAMDISK} -

MDSET.-.suffixes= srec bin
MDSET.-.post=	${OBJCOPY} -O srec netbsd.tmp netbsd.srec && \
		    gzip -9f netbsd.srec ; \
		${OBJCOPY} -O binary netbsd.tmp netbsd.bin && \
		    gzip -9f netbsd.bin

MDSETDIR=	installation/instkernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
