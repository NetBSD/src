#	$NetBSD: Makefile,v 1.3.26.1 2019/06/10 21:42:28 christos Exp $
CDBASE=		pmaxcd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
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
