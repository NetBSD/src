#	$NetBSD: Makefile,v 1.2 2007/03/06 21:52:46 bouyer Exp $
#

.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		boot-com			# gives ${CDBASE}.iso
CDKERNELS=	netbsd-INSTALL_LARGE.gz	netbsd	# from ../instkernel
CDKERNELS+=	netbsd-INSTALL_LAPTOP.gz nbsd-l
CDKERNELS+=	netbsd-INSTALL.gz	nbsd-i
CDBOOTOPTIONS=	-o console=com0

.include "${.CURDIR}/../../../common/Makefile.bootcd"
