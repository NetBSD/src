#	$NetBSD: Makefile,v 1.31 1998/10/02 20:20:29 drochner Exp $

SUBDIR=	adosfs arch compat dev filecorefs isofs miscfs msdosfs net \
	netatalk netccitt netinet netiso netnatm netns nfs sys ufs uvm vm

.if (${MACHINE_ARCH} != "alpha" && ${MACHINE_ARCH} != "mips" && \
     ${MACHINE_ARCH} != "powerpc")
SUBDIR+= lkm
.endif

.include <bsd.kinc.mk>
