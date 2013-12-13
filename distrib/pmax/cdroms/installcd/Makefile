#	$NetBSD: Makefile,v 1.3 2013/12/13 10:27:56 mrg Exp $
CDBASE=		pmaxcd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
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
