#	$NetBSD: Makefile,v 1.1.14.1 2008/06/02 13:21:12 mjf Exp $
CDBASE=		arccd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDKERNELS=	netbsd-RAMDISK.gz netbsd
CDINSTKERNEL=	../../instkernel
CDBUILDEXTRA=	${DESTDIR}/usr/mdec/boot

.include "${.CURDIR}/../../../common/Makefile.bootcd"
