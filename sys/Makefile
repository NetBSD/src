#	$NetBSD: Makefile,v 1.33 1999/03/10 06:47:57 ross Exp $

SUBDIR=	adosfs arch compat dev filecorefs isofs miscfs msdosfs net \
	netatalk netccitt netinet netiso netnatm netns nfs sys ufs uvm vm

.if ${MACHINE_ARCH} != "powerpc" && \
    ${MACHINE_ARCH} != "mipseb"  &&${MACHINE_ARCH} != "mipsel")

SUBDIR+= lkm

.endif

.include <bsd.kinc.mk>
