#	$NetBSD: Makefile,v 1.1 2006/10/29 17:58:15 hubertf Exp $
#

# Install CD, to be made after 'build.sh release'
# Requires populated $RELEASEDIR/${MACHINE}

CDBASE=		boot			# gives ${CDBASE}.iso
CDKERNEL=	netbsd-INSTALL.gz	# from ../instkernel
CDRELEASE=      true                    # include $RELEASEDIR/$MACHINE

.include "${.CURDIR}/../../../common/Makefile.bootcd"
