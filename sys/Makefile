#	$NetBSD: Makefile,v 1.29 1998/06/12 23:22:30 cgd Exp $

SUBDIR=	adosfs arch dev isofs miscfs msdosfs net netatalk netccitt netinet \
	netiso netnatm netns nfs sys ufs uvm vm

.if (${MACHINE_ARCH} != "alpha" && ${MACHINE_ARCH} != "mips" && \
     ${MACHINE_ARCH} != "powerpc")
SUBDIR+= lkm
.endif

.include <bsd.kinc.mk>
