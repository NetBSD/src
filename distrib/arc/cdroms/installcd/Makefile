#	$NetBSD: Makefile,v 1.3.64.1 2019/06/10 21:42:18 christos Exp $
CDBASE=		arccd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDRELEASE_NODEBUG=	true
CDKERNELS=	netbsd-RAMDISK.gz netbsd
CDINSTKERNEL=	../../instkernel
CDBUILDEXTRA=	${DESTDIR}/usr/mdec/boot

.include "${.CURDIR}/../../../common/Makefile.bootcd"
