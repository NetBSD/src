#	$NetBSD: Makefile,v 1.40.2.6 2002/02/28 04:05:48 nathanw Exp $

SUBDIR=	adosfs altq arch compat dev filecorefs fs isofs lkm miscfs msdosfs \
	net netatalk netccitt netinet netinet6 netisdn netiso netkey \
	netnatm netns netsmb nfs ntfs sys ufs uvm

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@true

.include <bsd.kinc.mk>
