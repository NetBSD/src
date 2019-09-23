#	$NetBSD: Makefile,v 1.5 2019/09/23 13:42:34 christos Exp $
CDBASE=		pmaxcd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$RELEASEMACHINEDIR
CDRELEASE_NODEBUG=	true
.if ${MACHINE_ARCH} == "mipsel"
CDKERNELS=	netbsd-INSTALL.gz netbsd
.else
CDKERNELS=	netbsd-INSTALL64.gz netbsd
.endif
CDINSTKERNEL=	../../instkernel

#make the CD bootable
image_md_post:
	${TOOL_INSTALLBOOT} -m${MACHINE} ${CDIMAGE} bootxx.${MACHINE}

.include "${.CURDIR}/../../../common/Makefile.bootcd"
