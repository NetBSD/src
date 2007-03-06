#	$NetBSD: Makefile,v 1.1 2007/03/06 21:57:27 bouyer Exp $
CDBASE=		vaxcd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDKERNELS=	install.ram.gz netbsd
CDINSTKERNEL=	../../ramdisk-kernel

#make the CD bootable
prepare_md_post:
	${RM} -f cdrom/boot
	${CP} ${DESTDIR}/usr/mdec/boot cdrom/boot
	${RM} -f bootxx.${MACHINE}
	${CP} ${DESTDIR}/usr/mdec/xxboot bootxx.${MACHINE}

image_md_post:
	${TOOL_INSTALLBOOT} -m${MACHINE} ${CDBASE}.iso bootxx.${MACHINE}

.include "${.CURDIR}/../../../common/Makefile.bootcd"
