#	$NetBSD: Makefile,v 1.1.4.1 2007/03/09 06:59:38 riz Exp $
#

.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		boot-com			# gives ${CDBASE}.iso
CDKERNELS=	netbsd-INSTALL.gz	netbsd	# from ../instkernel
CDBOOTOPTIONS=	-o console=com0

.include "${.CURDIR}/../../../common/Makefile.bootcd"
