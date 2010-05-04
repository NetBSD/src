#	$NetBSD: Makefile,v 1.2 2010/05/04 14:09:11 tsutsui Exp $
CDBASE=		macppccd		# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDKERNELS=	netbsd-INSTALL.gz netbsd
CDINSTKERNEL=	../../floppies/md-kernel
CDBUILDEXTRA=	${DESTDIR}/usr/mdec/ofwboot.xcf

.include "${.CURDIR}/../../../common/Makefile.bootcd"
