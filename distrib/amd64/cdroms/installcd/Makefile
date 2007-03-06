#	$NetBSD: Makefile,v 1.2 2007/03/06 21:52:45 bouyer Exp $
#

.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		amd64cd			# gives ${CDBASE}.iso
CDKERNELS=	netbsd-INSTALL.gz	netbsd	# from ../instkernel
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE

.include "${.CURDIR}/../../../common/Makefile.bootcd"
