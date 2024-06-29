#	$NetBSD: Makefile,v 1.22.28.1 2024/06/29 19:43:26 perseant Exp $

.include <bsd.own.mk>

SUBDIR=	adosfs autofs cd9660 efs filecorefs hfs msdosfs nilfs ntfs ptyfs \
	puffs sysvbfs tmpfs udf union v7fs

.if ${MKEXFATFS} != "no"
SUBDIR+= exfatfs
.endif

.include <bsd.kinc.mk>
