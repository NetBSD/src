#	$NetBSD: Makefile,v 1.37 1999/11/08 00:16:40 mycroft Exp $

SUBDIR=	adosfs arch compat dev filecorefs isofs miscfs msdosfs net \
	netatalk netccitt netinet netinet6 netiso netkey netnatm netns \
	nfs ntfs sys ufs uvm vm

.if (${MACHINE_ARCH} != "mipseb" && ${MACHINE_ARCH} != "mipsel")

SUBDIR+= lkm

.endif

.include <bsd.kinc.mk>
