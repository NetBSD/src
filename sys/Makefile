#	$NetBSD: Makefile,v 1.30 1998/08/14 13:05:37 augustss Exp $

SUBDIR=	adosfs arch dev filecorefs isofs miscfs msdosfs net netatalk netccitt \
	netinet netiso netnatm netns nfs sys ufs uvm vm

.if (${MACHINE_ARCH} != "alpha" && ${MACHINE_ARCH} != "mips" && \
     ${MACHINE_ARCH} != "powerpc")
SUBDIR+= lkm
.endif

.include <bsd.kinc.mk>
