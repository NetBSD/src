#	$NetBSD: Makefile,v 1.1.22.1 2014/11/08 11:35:45 msaitoh Exp $
CDBASE=		alphacd		# gives ${CDBASE}.iso
CDINSTKERNEL=	../../instkernel/instkernel
CDKERNELS=	netbsd netbsd
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE

#make the CD bootable
image_md_post:
	${TOOL_INSTALLBOOT} -m${MACHINE} ${CDIMAGE} bootxx.${MACHINE}

.include "${.CURDIR}/../../../common/Makefile.bootcd"

