#	$NetBSD: Makefile,v 1.2 2012/06/19 13:01:53 minoura Exp $
CDBASE=		x68kcd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDINSTKERNEL=	../../floppies/instkernel
CDKERNELS=	netbsd-INSTALL.gz	netbsd
BOOTXX_CD9660=	${DESTDIR}/usr/mdec/xxboot_cd9660
CDMAKEFSOPTIONS= generic-bootimage=${BOOTXX_CD9660}

prepare_md_post:
	${CP} ${BOOT} cdrom


.include "${.CURDIR}/../../../common/Makefile.bootcd"
