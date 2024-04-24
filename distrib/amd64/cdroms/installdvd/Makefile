#	$NetBSD: Makefile,v 1.1 2024/04/24 11:29:34 nia Exp $
#

# Install CD, to be made after 'build.sh release'
# Requires populated $RELEASEDIR/${MACHINE}

.include <bsd.own.mk>
.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		amd64dvd		# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$RELEASEMACHINEDIR
CDBUILDEXTRA=	boot.cfg		# Add boot.cfg file
CLEANFILES+=	boot.cfg

prepare_md_post:
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" \
		< ${.CURDIR}/boot.cfg.in > boot.cfg

.include "${.CURDIR}/../../../common/Makefile.bootcd"
