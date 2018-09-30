#	$NetBSD: Makefile,v 1.2.46.1 2018/09/30 01:45:03 pgoyette Exp $
CDBASE=		macppccd		# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDRELEASE_NODEBUG=	true
CDKERNELS=	netbsd-INSTALL.gz netbsd
CDINSTKERNEL=	../../floppies/md-kernel
CDBUILDEXTRA=	${DESTDIR}/usr/mdec/ofwboot.xcf

.include "${.CURDIR}/../../../common/Makefile.bootcd"
