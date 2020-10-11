#	$NetBSD: Makefile,v 1.1 2020/10/11 14:24:50 jmcneill Exp $

.include <bsd.own.mk>
.include "${.CURDIR}/Makefile.cdrom"

CDBASE=		evbarm-${MACHINE_ARCH}cd	# gives ${CDBASE}.iso
CDRELEASE=	true				# include $RELEASEDIR/$RELEASEMACHINEDIR
CDRELEASE_NODEBUG=	true
CLEANFILES+=	boot.cfg

prepare_md_post:
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" \
		< ${.CURDIR}/boot.cfg.in > boot.cfg

.include "${.CURDIR}/../../common/Makefile.bootcd"
