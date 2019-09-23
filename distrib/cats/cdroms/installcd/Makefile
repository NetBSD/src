#	$NetBSD: Makefile,v 1.4 2019/09/23 13:42:31 christos Exp $
CDBASE=		catscd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$RELEASEMACHINEDIR
CDRELEASE_NODEBUG=	true
CDKERNELS=	netbsd-INSTALL.aout netbsd
CDINSTKERNEL=	../../instkernel

.include "${.CURDIR}/../../../common/Makefile.bootcd"
