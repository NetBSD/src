#	$NetBSD: Makefile,v 1.36 1999/07/01 08:12:45 itojun Exp $

SUBDIR=	adosfs arch compat dev filecorefs isofs miscfs msdosfs net \
	netatalk netccitt netinet netinet6 netiso netkey netnatm netns \
	nfs ntfs sys ufs uvm vm

.if (${MACHINE_ARCH} != "powerpc" && \
    ${MACHINE_ARCH} != "mipseb" && ${MACHINE_ARCH} != "mipsel")

SUBDIR+= lkm

.endif

.include <bsd.kinc.mk>
