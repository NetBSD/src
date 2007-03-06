#	$NetBSD: Makefile,v 1.2 2007/03/06 21:52:46 bouyer Exp $
#

# Install CD, to be made after 'build.sh release'
# Requires populated $RELEASEDIR/${MACHINE}

.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		i386cd			# gives ${CDBASE}.iso
CDKERNELS=	netbsd-INSTALL_LARGE.gz	netbsd	# from ../instkernel
CDKERNELS+=	netbsd-INSTALL_LAPTOP.gz nbsd-l
CDKERNELS+=	netbsd-INSTALL.gz	nbsd-i
CDRELEASE=      true                    # include $RELEASEDIR/$MACHINE

.include "${.CURDIR}/../../../common/Makefile.bootcd"
