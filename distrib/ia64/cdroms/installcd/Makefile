#	$NetBSD: Makefile,v 1.3.2.2 2025/06/27 12:03:23 jdc Exp $
CDBASE=		ia64cd		# gives ${CDBASE}.iso

#CDINSTKERNEL=	../../instkernel/instkernel
CDKERNELS=	${KERNELCOMPDIR}/GENERIC/netbsd netbsd
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDRELEASE_NODEBUG=	true

# don't make the CD bootable, there is no installboot support for ia64
# image_md_post:
#	${TOOL_INSTALLBOOT} -m${MACHINE} ${CDIMAGE} bootxx.${MACHINE}

.include <bsd.own.mk>
KERNELCOMPDIR!= cd ${.CURDIR}/../../../../sys/arch/${MACHINE}/compile && ${PRINTOBJDIR}

.include "${.CURDIR}/../../../common/Makefile.bootcd"

