#	$NetBSD: Makefile,v 1.3 2018/09/28 15:05:20 martin Exp $
CDBASE=		catscd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDRELEASE_NODEBUG=	true
CDKERNELS=	netbsd-INSTALL.aout netbsd
CDINSTKERNEL=	../../instkernel

.include "${.CURDIR}/../../../common/Makefile.bootcd"
