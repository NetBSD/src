#	$NetBSD: Makefile,v 1.1 2020/03/21 14:25:49 tsutsui Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-${MACHINE}-live	# gives ${IMGBASE}.img

.include <bsd.own.mk>

LIVEIMAGEMB?=	1920		# for 2GB SD due to Windows Mobile restriction
SWAPMB?=	64
FATMB?=		32

USE_MBR=	yes
MBRFAT=		6		# 16-bit FAT, more than 32M

# Omit comp etc. due to size restriction
KERN_SET=	kern-WZERO3
#SETS=		base modules etc misc rescue text xbase xetc xfont xserver

# prepare JORNADA720 kernel as an independent name
prepare_md_post:
	echo Extracting kern-JORNADA720.${TAR_SUFF} ...
	${TOOL_PAX} ${PAX_TIMESTAMP} -rn				\
	    --use-compress-program=${COMPRESS_PROGRAM:Q}		\
	    -f ${SETS_DIR}/kern-JORNADA720.${TAR_SUFF} .
	mv netbsd netbsd.JORNADA720

CLEANFILES+=	netbsd.JORNADA720

IMGFILE_EXTRA=								\
		${.OBJDIR}/netbsd.JORNADA720	.

DISKPROTO_IN=	${.CURDIR}/../../common/bootimage/diskproto.mbrfat.in
FSTAB_IN=	${.CURDIR}/fstab.in
SPEC_EXTRA=	${.CURDIR}/spec.in

FATFILES=	${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/hpcboot.exe
FATFILES+=	${WORKDIR}/netbsd
FATFILES+=	${WORKDIR}/netbsd.JORNADA720

.include "${.CURDIR}/../../common/bootimage/Makefile.liveimage"
