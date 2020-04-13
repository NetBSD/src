#	$NetBSD: Makefile,v 1.3.26.2 2020/04/13 07:45:30 martin Exp $
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
