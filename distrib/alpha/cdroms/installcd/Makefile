#	$NetBSD: Makefile,v 1.2.42.1 2019/06/10 21:42:16 christos Exp $
CDBASE=		alphacd		# gives ${CDBASE}.iso
CDINSTKERNEL=	../../instkernel/instkernel
CDKERNELS=	netbsd netbsd
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDRELEASE_NODEBUG=	true

#make the CD bootable
image_md_post:
	${TOOL_INSTALLBOOT} -m${MACHINE} ${CDIMAGE} bootxx.${MACHINE}

.include "${.CURDIR}/../../../common/Makefile.bootcd"

