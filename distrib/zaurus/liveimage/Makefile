#	$NetBSD: Makefile,v 1.1 2020/03/21 14:24:12 tsutsui Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-zaurus-live	# gives ${IMGBASE}.img

.include <bsd.own.mk>

LIVEIMAGEMB?=	900		# for "1GB" SD due to Zaurus Linux restriction
SWAPMB?=	64
FATMB?=		32

USE_MBR=	yes
MBRFAT=		6		# 16-bit FAT, more than 32M

# Omit comp etc. due to size restriction
KERN_SET=	kern-GENERIC
SETS=		base modules etc misc rescue text xbase xetc xfont xserver

# prepare SL-C700 kernel as an independent name
prepare_md_post:
	echo Extracting kern-C700.${TAR_SUFF} ...
	${TOOL_PAX} ${PAX_TIMESTAMP} -rn				\
	    --use-compress-program=${COMPRESS_PROGRAM:Q}		\
	    -f ${SETS_DIR}/kern-C700.${TAR_SUFF} .
	mv netbsd netbsd.c700

CLEANFILES+=	netbsd.c700

IMGFILE_EXTRA=								\
		${.OBJDIR}/netbsd.c700		.

DISKPROTO_IN=	${.CURDIR}/../../common/bootimage/diskproto.mbrfat.in
FSTAB_IN=	${.CURDIR}/fstab.in
SPEC_EXTRA=	${.CURDIR}/spec.in

FATFILES=	${WORKDIR}/usr/mdec/zbsdmod.o ${WORKDIR}/usr/mdec/zboot
FATFILES+=	${WORKDIR}/netbsd
FATFILES+=	${WORKDIR}/netbsd.c700

.include "${.CURDIR}/../../common/bootimage/Makefile.liveimage"
