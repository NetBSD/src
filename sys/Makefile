#	$NetBSD: Makefile,v 1.34.4.1 1999/06/21 00:44:00 thorpej Exp $

SUBDIR=	adosfs arch compat dev filecorefs isofs miscfs msdosfs net \
	netatalk netccitt netinet netiso netnatm netns nfs ntfs sys ufs uvm vm

.if (${MACHINE_ARCH} != "powerpc" && \
    ${MACHINE_ARCH} != "mipseb" && ${MACHINE_ARCH} != "mipsel")

SUBDIR+= lkm

.endif

.include <bsd.kinc.mk>
