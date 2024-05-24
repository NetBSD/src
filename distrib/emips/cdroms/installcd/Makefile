#	$NetBSD: Makefile,v 1.5 2024/05/24 13:26:35 tsutsui Exp $
CDBASE=		emipscd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$RELEASEMACHINEDIR
CDRELEASE_NODEBUG=	true
CDKERNELS=	netbsd-INSTALL.gz netbsd
CDINSTKERNEL=	../../instkernel

.include "${.CURDIR}/../../../common/Makefile.bootcd"
