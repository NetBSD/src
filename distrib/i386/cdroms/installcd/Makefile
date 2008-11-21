#	$NetBSD: Makefile,v 1.6 2008/11/21 15:49:51 ad Exp $
#

# Install CD, to be made after 'build.sh release'
# Requires populated $RELEASEDIR/${MACHINE}

.include <bsd.own.mk>
.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		i386cd			# gives ${CDBASE}.iso
CDRELEASE=	true                    # include $RELEASEDIR/$MACHINE
CDBUILDEXTRA+=	boot.cfg		# Add boot.cfg file
CLEANFILES+=	boot.cfg
SYSINSTDIR!=	cd ${.CURDIR}/../../../utils/sysinst/arch/${MACHINE} && ${PRINTOBJDIR}

prepare_md_post:
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" < ${.CURDIR}/boot.cfg.in > boot.cfg

image_md_pre:
	${CP} ${.CURDIR}/../../ramdisks/common/dot.profile cdrom/etc/rc
	${MKDIR} cdrom/mnt2 cdrom/targetroot
	${CP} ${SYSINSTDIR}/sysinst cdrom
	${CHMOD} ugo+rx cdrom/sysinst
	${CP} ${SYSINSTDIR}/sysinstmsgs.?? cdrom

.include "${.CURDIR}/../../../common/Makefile.bootcd"
