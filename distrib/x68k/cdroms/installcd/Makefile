#	$NetBSD: Makefile,v 1.5 2019/09/23 13:42:36 christos Exp $
CDBASE=		x68kcd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$RELEASEMACHINEDIR
CDRELEASE_NODEBUG=	true
CDINSTKERNEL=	../../floppies/instkernel
CDKERNELS=	netbsd-INSTALL.gz	netbsd
CDBUILDEXTRA=	${BOOT}
BOOTXX_CD9660=	${DESTDIR}/usr/mdec/xxboot_cd9660
CDMAKEFSOPTIONS= generic-bootimage=${BOOTXX_CD9660}

.include "${.CURDIR}/../../../common/Makefile.bootcd"
