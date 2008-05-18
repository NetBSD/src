#	$NetBSD: Makefile,v 1.1.16.1 2008/05/18 12:29:38 yamt Exp $
CDBASE=		prepcd		# gives ${CDBASE}.iso
CDRELEASE=	true		# include $RELEASEDIR/$MACHINE
CDMAKEFSOPTIONS=bootimage=powerpc;cdrom/prep/installation/floppy/sysinst_com0.fs

# prep needs a fdisk partition table at the head of the cd, pointing to the
# boot image.  We create an el-torrito cd, and then point partition 0 at
# it.

image_md_post:
	${TOOL_FDISK} -F -f -b 1/255/16065 -0 -s 65/80/2799 -i ${CDBASE}.iso
	${TOOL_FDISK} -F -f -b 1/255/16065 -0 -s 65/80/2799 -u ${CDBASE}.iso

.include "${.CURDIR}/../../../common/Makefile.bootcd"
