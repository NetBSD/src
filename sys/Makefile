#	$NetBSD: Makefile,v 1.34 1999/03/10 08:56:16 tls Exp $

SUBDIR=	adosfs arch compat dev filecorefs isofs miscfs msdosfs net \
	netatalk netccitt netinet netiso netnatm netns nfs sys ufs uvm vm

.if (${MACHINE_ARCH} != "powerpc" && \
    ${MACHINE_ARCH} != "mipseb" && ${MACHINE_ARCH} != "mipsel")

SUBDIR+= lkm

.endif

.include <bsd.kinc.mk>
