#	$NetBSD: Makefile,v 1.3 2018/09/28 15:05:21 martin Exp $
CDBASE=		macppccd		# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDRELEASE_NODEBUG=	true
CDKERNELS=	netbsd-INSTALL.gz netbsd
CDINSTKERNEL=	../../floppies/md-kernel
CDBUILDEXTRA=	${DESTDIR}/usr/mdec/ofwboot.xcf

.include "${.CURDIR}/../../../common/Makefile.bootcd"
