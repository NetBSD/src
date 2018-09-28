#	$NetBSD: Makefile,v 1.3 2018/09/28 15:05:20 martin Exp $
CDBASE=		emipscd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDRELEASE_NODEBUG=	true
CDKERNELS=	netbsd-INSTALL.gz netbsd
CDINSTKERNEL=	../../instkernel

#make the CD bootable
image_md_post:
	${TOOL_INSTALLBOOT} -m${MACHINE} ${CDIMAGE} bootxx.${MACHINE}

.include "${.CURDIR}/../../../common/Makefile.bootcd"
