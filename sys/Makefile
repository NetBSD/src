#	$NetBSD: Makefile,v 1.40.4.1 2001/08/25 06:14:57 thorpej Exp $

SUBDIR=	adosfs altq arch compat dev filecorefs isofs miscfs msdosfs net \
	netatalk netccitt netinet netinet6 netisdn netiso netkey netnatm \
	netns nfs ntfs sys ufs uvm

.if (${MACHINE_ARCH} != "mipseb" && ${MACHINE_ARCH} != "mipsel")

SUBDIR+= lkm

.endif

.include <bsd.kinc.mk>
