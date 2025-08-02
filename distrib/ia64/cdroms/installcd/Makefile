#	$NetBSD: Makefile,v 1.4.10.1 2025/08/02 05:20:21 perseant Exp $
CDBASE=		ia64cd		# gives ${CDBASE}.iso

#CDINSTKERNEL=	../../instkernel/instkernel
CDKERNELS=	${KERNELCOMPDIR}/GENERIC/netbsd netbsd
CDRELEASE=	true			# include $RELEASEDIR/$RELEASEMACHINEDIR
CDRELEASE_NODEBUG=	true

# don't make the CD bootable, there is no installboot support for ia64
# image_md_post:
#	${TOOL_INSTALLBOOT} -m${MACHINE} ${CDIMAGE} bootxx.${MACHINE}

.include <bsd.own.mk>
KERNELCOMPDIR!= cd ${.CURDIR}/../../../../sys/arch/${MACHINE}/compile && ${PRINTOBJDIR}

.include "${.CURDIR}/../../../common/Makefile.bootcd"

