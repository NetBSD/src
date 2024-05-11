#	$NetBSD: Makefile,v 1.10.8.1 2024/05/11 14:31:57 martin Exp $
#

# Install CD, to be made after 'build.sh release'
# Requires populated $RELEASEDIR/${MACHINE}

.include <bsd.own.mk>
.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		i386cd			# gives ${CDBASE}.iso
CDRELEASE=	true                    # include $RELEASEDIR/$RELEASEMACHINEDIR
CDRELEASE_NODEBUG=	true
CDBUILDEXTRA+=	boot.cfg		# Add boot.cfg file
CLEANFILES+=	boot.cfg

# already (probably) booting from CD if using this and the floppy images
# are 20MB+ when combined
CDRELEASE_EXCLUDE+=	-s ',./installation/floppy/.*,,gp'

# MONOLITHIC (i386-only) was originally added to the release build
# for upgrades from netbsd-5, and now primarily exists so the no-modules
# build can continue to be tested.  this takes up 20MB+ space.
.  for sufx in tgz tar.xz
CDRELEASE_EXCLUDE+=	-s ',./binary/sets/kern-INSTALL.${sufx},,gp'
CDRELEASE_EXCLUDE+=	-s ',./binary/sets/kern-MONOLITHIC.${sufx},,gp'
.  endfor

CDRELEASE_EXCLUDE+=	-s ',./binary/kernel/netbsd-INSTALL.gz,,gp'
CDRELEASE_EXCLUDE+=	-s ',./binary/kernel/netbsd-MONOLITHIC.gz,,gp'

prepare_md_post:
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" < ${.CURDIR}/boot.cfg.in > boot.cfg

.include "${.CURDIR}/../../../common/Makefile.bootcd"
