#	$NetBSD: Makefile,v 1.3 2008/05/01 21:59:07 xtraeme Exp $
#

.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		amd64cd			# gives ${CDBASE}.iso
CDKERNELS=	netbsd-INSTALL.gz	netbsd	# from ../instkernel
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDBUILDEXTRA=	boot.cfg		# Add boot.cfg file
CLEANFILES+=	boot.cfg

prepare_md_post:
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" \
		< ${.CURDIR}/boot.cfg.in > boot.cfg

.include "${.CURDIR}/../../../common/Makefile.bootcd"
