#	$NetBSD: Makefile,v 1.3 2008/10/22 11:59:24 tsutsui Exp $
#

.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		boot-com			# gives ${CDBASE}.iso
CDKERNELS=	netbsd-INSTALL.gz	netbsd	# from ../instkernel
CDBOOTOPTIONS=	-o console=com0

CDBUILDEXTRA+=	boot.cfg		# Add boot..cfg file
CLEANFILES+=	boot.cfg

prepare_md_post:
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" < ${.CURDIR}/../bootcd/boot.cfg.in > boot.cfg

.include "${.CURDIR}/../../../common/Makefile.bootcd"
