#	$NetBSD: Makefile,v 1.36.2.1 2000/11/20 22:19:20 bouyer Exp $

SUBDIR=	adosfs arch compat dev filecorefs isofs miscfs msdosfs net \
	netatalk netccitt netinet netinet6 netiso netkey netnatm netns \
	nfs ntfs sys ufs uvm

.if (${MACHINE_ARCH} != "mipseb" && ${MACHINE_ARCH} != "mipsel")

SUBDIR+= lkm

.endif

.include <bsd.kinc.mk>
