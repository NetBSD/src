#	$NetBSD: Makefile,v 1.2 2012/01/15 22:34:33 jmcneill Exp $
CDBASE=		emipscd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDKERNELS=	netbsd-INSTALL.gz netbsd
CDINSTKERNEL=	../../instkernel

#make the CD bootable
image_md_post:
	${TOOL_INSTALLBOOT} -m${MACHINE} ${CDIMAGE} bootxx.${MACHINE}

.include "${.CURDIR}/../../../common/Makefile.bootcd"
