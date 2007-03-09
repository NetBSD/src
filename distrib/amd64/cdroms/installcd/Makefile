#	$NetBSD: Makefile,v 1.1.2.1 2007/03/09 06:59:39 riz Exp $
#

.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		amd64cd			# gives ${CDBASE}.iso
CDKERNELS=	netbsd-INSTALL.gz	netbsd	# from ../instkernel
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE

.include "${.CURDIR}/../../../common/Makefile.bootcd"
