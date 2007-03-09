#	$NetBSD: Makefile,v 1.1.2.1 2007/03/09 06:59:30 riz Exp $
#

# Install CD, to be made after 'build.sh release'
# Requires populated $RELEASEDIR/${MACHINE}

.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		i386cd			# gives ${CDBASE}.iso
CDRELEASE=      true                    # include $RELEASEDIR/$MACHINE

.include "${.CURDIR}/../../../common/Makefile.bootcd"
