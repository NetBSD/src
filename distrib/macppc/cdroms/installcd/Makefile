#	$NetBSD: Makefile,v 1.2.48.2 2020/04/13 07:45:29 martin Exp $
CDBASE=		macppccd		# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$RELEASEMACHINEDIR
CDRELEASE_NODEBUG=	true
CDKERNELS=	netbsd-INSTALL.gz netbsd
CDKERNELS+=	netbsd-INSTALL_601.gz netbsd.601
CDINSTKERNEL=	../../floppies/md-kernel
CDBUILDEXTRA=	${DESTDIR}/usr/mdec/ofwboot.xcf

.include "${.CURDIR}/../../../common/Makefile.bootcd"
