#	$NetBSD: Makefile,v 1.41.2.1 2001/10/01 12:36:47 fvdl Exp $

SUBDIR=	adosfs altq arch compat dev filecorefs isofs miscfs msdosfs net \
	netatalk netccitt netinet netinet6 netisdn netiso netkey netnatm \
	netns nfs ntfs sys ufs uvm

.if (${MACHINE_ARCH} != "mipseb" && ${MACHINE_ARCH} != "mipsel")

SUBDIR+= lkm

.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@${TRUE}

.include <bsd.kinc.mk>
