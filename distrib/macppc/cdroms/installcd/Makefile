#	$NetBSD: Makefile,v 1.2.48.1 2019/06/10 21:42:24 christos Exp $
CDBASE=		macppccd		# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDRELEASE_NODEBUG=	true
CDKERNELS=	netbsd-INSTALL.gz netbsd
CDKERNELS+=	netbsd-INSTALL_601.gz netbsd.601
CDINSTKERNEL=	../../floppies/md-kernel
CDBUILDEXTRA=	${DESTDIR}/usr/mdec/ofwboot.xcf

.include "${.CURDIR}/../../../common/Makefile.bootcd"
