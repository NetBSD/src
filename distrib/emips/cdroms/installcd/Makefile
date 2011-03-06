#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:37 bouyer Exp $
CDBASE=		emipscd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDKERNELS=	netbsd-INSTALL.gz netbsd
CDINSTKERNEL=	../../instkernel

#make the CD bootable
image_md_post:
	${TOOL_INSTALLBOOT} -m${MACHINE} ${CDBASE}.iso bootxx.${MACHINE}

.include "${.CURDIR}/../../../common/Makefile.bootcd"
