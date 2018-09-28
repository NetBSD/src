#	$NetBSD: Makefile,v 1.9 2018/09/28 15:05:21 martin Exp $
#

# Install CD, to be made after 'build.sh release'
# Requires populated $RELEASEDIR/${MACHINE}

.include <bsd.own.mk>
.include "${.CURDIR}/../Makefile.cdrom"

CDBASE=		i386cd			# gives ${CDBASE}.iso
CDRELEASE=	true                    # include $RELEASEDIR/$MACHINE
CDRELEASE_NODEBUG=	true
CDBUILDEXTRA+=	boot.cfg		# Add boot.cfg file
CLEANFILES+=	boot.cfg

prepare_md_post:
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" < ${.CURDIR}/boot.cfg.in > boot.cfg

.include "${.CURDIR}/../../../common/Makefile.bootcd"
