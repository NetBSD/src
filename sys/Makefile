#	$NetBSD: Makefile,v 1.32 1999/03/03 12:00:20 jonathan Exp $

SUBDIR=	adosfs arch compat dev filecorefs isofs miscfs msdosfs net \
	netatalk netccitt netinet netiso netnatm netns nfs sys ufs uvm vm

.if (${MACHINE_ARCH} != "alpha" && ${MACHINE_ARCH} != "powerpc" && \
    ${MACHINE_ARCH} != "mipseb"  &&${MACHINE_ARCH} != "mipsel")

SUBDIR+= lkm
.endif

.include <bsd.kinc.mk>
