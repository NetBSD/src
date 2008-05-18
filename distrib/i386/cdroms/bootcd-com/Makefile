#	$NetBSD: Makefile,v 1.4.2.1 2008/05/18 12:29:35 yamt Exp $
#

.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		boot-com			# gives ${CDBASE}.iso
CDBOOTOPTIONS=	-o console=com0

CDBUILDEXTRA+=	boot.cfg		# Add boot.cfg file
CLEANFILES+=	boot.cfg

prepare_md_post:
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" < ${.CURDIR}/../bootcd/boot.cfg.in > boot.cfg

.include "${.CURDIR}/../../../common/Makefile.bootcd"
