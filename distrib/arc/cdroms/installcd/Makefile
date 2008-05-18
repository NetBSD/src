#	$NetBSD: Makefile,v 1.1.16.1 2008/05/18 12:29:34 yamt Exp $
CDBASE=		arccd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDKERNELS=	netbsd-RAMDISK.gz netbsd
CDINSTKERNEL=	../../instkernel
CDBUILDEXTRA=	${DESTDIR}/usr/mdec/boot

.include "${.CURDIR}/../../../common/Makefile.bootcd"
