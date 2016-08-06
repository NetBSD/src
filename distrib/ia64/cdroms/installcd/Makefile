#	$NetBSD: Makefile,v 1.1.2.2 2016/08/06 00:18:39 pgoyette Exp $
CDBASE=		ia64cd		# gives ${CDBASE}.iso
#CDINSTKERNEL=	../../instkernel/instkernel
CDKERNELS=	netbsd netbsd
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE

#make the CD bootable
image_md_post:
	${TOOL_INSTALLBOOT} -m${MACHINE} ${CDIMAGE} bootxx.${MACHINE}

.include "${.CURDIR}/../../../common/Makefile.bootcd"

