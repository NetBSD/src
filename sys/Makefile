#	$NetBSD: Makefile,v 1.46 2002/01/09 21:51:53 deberg Exp $

SUBDIR=	adosfs altq arch compat dev filecorefs fs isofs lkm miscfs msdosfs \
	net netatalk netccitt netinet netinet6 netisdn netiso netkey \
	netnatm netns netsmb nfs ntfs sys ufs uvm

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@true

.include <bsd.kinc.mk>
