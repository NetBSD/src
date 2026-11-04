#	$NetBSD: Makefile,v 1.9.8.1 2026/11/04 22:36:51 perseant Exp $
#

CDMAKEFSEXTRAOPTS=	-m 700m

# HP Proliant iLO serial console is on com1
CDBOOTOPTIONS=	-o console=com0

.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		boot-com			# gives ${CDBASE}.iso

CDBUILDEXTRA+=	boot.cfg		# Add boot.cfg file
CLEANFILES+=	boot.cfg

prepare_md_post:
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" < ${.CURDIR}/../bootcd/boot.cfg.in > boot.cfg

.include "${.CURDIR}/../../../common/Makefile.bootcd"
