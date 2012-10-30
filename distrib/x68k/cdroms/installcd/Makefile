#	$NetBSD: Makefile,v 1.1.38.1 2012/10/30 18:48:56 yamt Exp $
CDBASE=		x68kcd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDINSTKERNEL=	../../floppies/instkernel
CDKERNELS=	netbsd-INSTALL.gz	netbsd
BOOTXX_CD9660=	${DESTDIR}/usr/mdec/xxboot_cd9660
CDMAKEFSOPTIONS= generic-bootimage=${BOOTXX_CD9660}

prepare_md_post:
	${CP} ${BOOT} cdrom


.include "${.CURDIR}/../../../common/Makefile.bootcd"
