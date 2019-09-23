#	$NetBSD: Makefile,v 1.4 2019/09/23 13:42:31 christos Exp $
CDBASE=		emipscd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$RELEASEMACHINEDIR
CDRELEASE_NODEBUG=	true
CDKERNELS=	netbsd-INSTALL.gz netbsd
CDINSTKERNEL=	../../instkernel

#make the CD bootable
image_md_post:
	${TOOL_INSTALLBOOT} -m${MACHINE} ${CDIMAGE} bootxx.${MACHINE}

.include "${.CURDIR}/../../../common/Makefile.bootcd"
