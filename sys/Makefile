#	$NetBSD: Makefile,v 1.35 1999/05/17 16:41:48 kleink Exp $

SUBDIR=	adosfs arch compat dev filecorefs isofs miscfs msdosfs net \
	netatalk netccitt netinet netiso netnatm netns nfs ntfs sys ufs uvm vm

.if (${MACHINE_ARCH} != "powerpc" && \
    ${MACHINE_ARCH} != "mipseb" && ${MACHINE_ARCH} != "mipsel")

SUBDIR+= lkm

.endif

.include <bsd.kinc.mk>
