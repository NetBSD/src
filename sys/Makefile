#	$NetBSD: Makefile,v 1.39 2000/12/15 01:30:36 thorpej Exp $

SUBDIR=	adosfs altq arch compat dev filecorefs isofs miscfs msdosfs net \
	netatalk netccitt netinet netinet6 netiso netkey netnatm netns \
	nfs ntfs sys ufs uvm

.if (${MACHINE_ARCH} != "mipseb" && ${MACHINE_ARCH} != "mipsel")

SUBDIR+= lkm

.endif

.include <bsd.kinc.mk>
