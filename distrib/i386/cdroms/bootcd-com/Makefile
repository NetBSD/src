#	$NetBSD: Makefile,v 1.7.62.1 2026/11/04 22:36:55 perseant Exp $
#

CDMAKEFSEXTRAOPTS=	-m 700m

.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		boot-com			# gives ${CDBASE}.iso
CDBOOTOPTIONS=	-o console=com0

CDBUILDEXTRA+=	boot.cfg		# Add boot.cfg file
CLEANFILES+=	boot.cfg

prepare_md_post:
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" < ${.CURDIR}/../bootcd/boot.cfg.in > boot.cfg

.include "${.CURDIR}/../../../common/Makefile.bootcd"
