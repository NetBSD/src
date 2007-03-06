#	$NetBSD: Makefile,v 1.2 2007/03/06 21:52:45 bouyer Exp $
#

.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		boot-com			# gives ${CDBASE}.iso
CDKERNELS=	netbsd-INSTALL.gz	netbsd	# from ../instkernel
CDBOOTOPTIONS=	-o console=com0

.include "${.CURDIR}/../../../common/Makefile.bootcd"
