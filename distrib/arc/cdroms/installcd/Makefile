#	$NetBSD: Makefile,v 1.4 2018/09/28 15:05:20 martin Exp $
CDBASE=		arccd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDRELEASE_NODEBUG=	true
CDKERNELS=	netbsd-RAMDISK.gz netbsd
CDINSTKERNEL=	../../instkernel
CDBUILDEXTRA=	${DESTDIR}/usr/mdec/boot

.include "${.CURDIR}/../../../common/Makefile.bootcd"
