#	$NetBSD: Makefile,v 1.6 2008/11/21 15:49:51 ad Exp $
#

.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		boot			# gives ${CDBASE}.iso
CDBUILDEXTRA+=	boot.cfg		# Add boot.cfg file
CLEANFILES+=	boot.cfg

prepare_md_post:
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" < ${.CURDIR}/boot.cfg.in > boot.cfg

image_md_pre:
	${CP} ${.CURDIR}/../../ramdisks/common/dot.profile cdrom/etc/rc

.include "${.CURDIR}/../../../common/Makefile.bootcd"
