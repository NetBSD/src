#	$NetBSD: Makefile,v 1.1 2007/03/06 21:57:16 bouyer Exp $
CDBASE=		pmaxcd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDKERNELS=	netbsd-INSTALL.gz netbsd
CDINSTKERNEL=	../../instkernel

#make the CD bootable
image_md_post:
	${TOOL_INSTALLBOOT} -m${MACHINE} ${CDBASE}.iso bootxx.${MACHINE}

.include "${.CURDIR}/../../../common/Makefile.bootcd"
