#	$NetBSD: Makefile,v 1.1.40.1 2012/11/24 20:29:49 jdc Exp $
CDBASE=		x68kcd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDINSTKERNEL=	../../floppies/instkernel
CDKERNELS=	netbsd-INSTALL.gz	netbsd
CDBUILDEXTRA=	${BOOT}
BOOTXX_CD9660=	${DESTDIR}/usr/mdec/xxboot_cd9660
CDMAKEFSOPTIONS= generic-bootimage=${BOOTXX_CD9660}

.include "${.CURDIR}/../../../common/Makefile.bootcd"
