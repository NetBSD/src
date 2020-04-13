#	$NetBSD: Makefile,v 1.3.64.2 2020/04/13 07:45:24 martin Exp $
CDBASE=		arccd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$RELEASEMACHINEDIR
CDRELEASE_NODEBUG=	true
CDKERNELS=	netbsd-RAMDISK.gz netbsd
CDINSTKERNEL=	../../instkernel
CDBUILDEXTRA=	${DESTDIR}/usr/mdec/boot

.include "${.CURDIR}/../../../common/Makefile.bootcd"
