#	$NetBSD: Makefile,v 1.2.14.2 2020/04/13 07:45:28 martin Exp $
CDBASE=		ia64cd		# gives ${CDBASE}.iso
#CDINSTKERNEL=	../../instkernel/instkernel
CDKERNELS=	netbsd netbsd
CDRELEASE=	true			# include $RELEASEDIR/$RELEASEMACHINEDIR
CDRELEASE_NODEBUG=	true

# don't make the CD bootable, there is no installboot support for ia64
# image_md_post:
#	${TOOL_INSTALLBOOT} -m${MACHINE} ${CDIMAGE} bootxx.${MACHINE}

.include "${.CURDIR}/../../../common/Makefile.bootcd"

