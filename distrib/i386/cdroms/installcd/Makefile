#	$NetBSD: Makefile,v 1.17 2026/01/07 16:08:22 nia Exp $
#

# Install CD, to be made after 'build.sh release'
# Requires populated $RELEASEDIR/${MACHINE}
# 
# Lots of effort to make this continue to fit within 700MB.

.include <bsd.own.mk>

CDMAKEFSEXTRAOPTS=	-m 700m

.include "${.CURDIR}/../../../common/Makefile.minibase"
.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		i386cd			# gives ${CDBASE}.iso
CDRELEASE=	true                    # include $RELEASEDIR/$RELEASEMACHINEDIR
CDRELEASE_NODEBUG=	true
CDRELEASE_NOHTML=	true
CDRELEASE_NOTESTS=	true
CDBUILDEXTRA+=	boot.cfg		# Add boot.cfg file
CLEANFILES+=	boot.cfg

# already (probably) booting from CD if using this and the floppy images
# are 20MB+ when combined
CDRELEASE_EXCLUDE+=	-s ',./installation/floppy/.*,,gp'

# MONOLITHIC (i386-only) was originally added to the release build
# for upgrades from netbsd-5, and now primarily exists so the no-modules
# build can continue to be tested.  this takes up 20MB+ space.
# 
# the INSTALL kernel is not used by the CD-ROMs, and MICROVM should be
# booted directly by QEMU rather than via the installer ISO.
CDRELEASE_EXCLUDE+=	-s ',./binary/sets/kern-INSTALL.*,,gp'
CDRELEASE_EXCLUDE+=	-s ',./binary/sets/kern-MONOLITHIC.*,,gp'
CDRELEASE_EXCLUDE+=	-s ',./binary/sets/kern-MICROVM.*,,gp'
CDRELEASE_EXCLUDE+=	-s ',./binary/kernel/netbsd-INSTALL.*,,gp'
CDRELEASE_EXCLUDE+=	-s ',./binary/kernel/netbsd-MONOLITHIC.*,,gp'
CDRELEASE_EXCLUDE+=	-s ',./binary/kernel/netbsd-MICROVM.*,,gp'

prepare_md_post:
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" < ${.CURDIR}/boot.cfg.in > boot.cfg

.include "${.CURDIR}/../../../common/Makefile.bootcd"
