#	$NetBSD: Makefile,v 1.3.40.1 2018/09/30 01:45:03 pgoyette Exp $
CDBASE=		prepcd		# gives ${CDBASE}.iso
CDRELEASE=	true		# include $RELEASEDIR/$MACHINE
CDRELEASE_NODEBUG=	true
CDMAKEFSOPTIONS=bootimage=powerpc;cdrom/prep/installation/floppy/sysinst_com0.fs

# prep needs a fdisk partition table at the head of the cd, pointing to the
# boot image.  We create an el-torrito cd, and then point partition 0 at
# it.

image_md_post:
	${TOOL_FDISK} -F -f -b 1/255/16065 -0 -s 65/80/2799 -i ${CDIMAGE}
	${TOOL_FDISK} -F -f -b 1/255/16065 -0 -s 65/80/2799 -u ${CDIMAGE}

.include "${.CURDIR}/../../../common/Makefile.bootcd"
