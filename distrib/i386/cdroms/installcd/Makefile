#	$NetBSD: Makefile,v 1.4.4.2 2007/11/23 16:21:23 sborrill Exp $
#

# Install CD, to be made after 'build.sh release'
# Requires populated $RELEASEDIR/${MACHINE}

.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		i386cd			# gives ${CDBASE}.iso
CDRELEASE=	true                    # include $RELEASEDIR/$MACHINE
CDBUILDEXTRA=	boot.cfg		# Add boot.cfg file
CLEANFILES+=	boot.cfg

prepare_md_post:
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" < ${.CURDIR}/boot.cfg.in > boot.cfg

.include "${.CURDIR}/../../../common/Makefile.bootcd"
