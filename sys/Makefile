#	$NetBSD: Makefile,v 1.34.6.1 1999/06/28 06:36:45 itojun Exp $

SUBDIR=	adosfs arch compat dev filecorefs isofs miscfs msdosfs net \
	netatalk netccitt netinet netiso netnatm netns nfs sys ufs uvm vm
SUBDIR+=netinet6 netkey

.if (${MACHINE_ARCH} != "powerpc" && \
    ${MACHINE_ARCH} != "mipseb" && ${MACHINE_ARCH} != "mipsel")

SUBDIR+= lkm

.endif

.include <bsd.kinc.mk>
