#	$NetBSD: Makefile,v 1.3.32.2 2020/04/13 07:45:37 martin Exp $
CDBASE=		x68kcd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$RELEASEMACHINEDIR
CDRELEASE_NODEBUG=	true
CDINSTKERNEL=	../../floppies/instkernel
CDKERNELS=	netbsd-INSTALL.gz	netbsd
CDBUILDEXTRA=	${BOOT}
BOOTXX_CD9660=	${DESTDIR}/usr/mdec/xxboot_cd9660
CDMAKEFSOPTIONS= generic-bootimage=${BOOTXX_CD9660}

.include "${.CURDIR}/../../../common/Makefile.bootcd"
