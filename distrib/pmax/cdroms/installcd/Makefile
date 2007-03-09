#	$NetBSD: Makefile,v 1.1.2.2 2007/03/09 06:59:53 riz Exp $
CDBASE=		pmaxcd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDKERNELS=	netbsd-INSTALL.gz netbsd
CDINSTKERNEL=	../../instkernel

#make the CD bootable
image_md_post:
	${TOOL_INSTALLBOOT} -m${MACHINE} ${CDBASE}.iso bootxx.${MACHINE}

.include "${.CURDIR}/../../../common/Makefile.bootcd"
