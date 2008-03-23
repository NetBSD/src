#	Makefile,v 1.3 2007/03/08 13:48:36 bouyer Exp
#

.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		boot			# gives ${CDBASE}.iso
CDBUILDEXTRA=	boot.cfg		# Add boot.cfg file
CLEANFILES+=	boot.cfg

prepare_md_post:
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" < ${.CURDIR}/boot.cfg.in > boot.cfg

.include "${.CURDIR}/../../../common/Makefile.bootcd"
