#	$NetBSD: Makefile,v 1.45 2001/12/02 00:47:21 jdolecek Exp $

SUBDIR=	adosfs altq arch compat dev filecorefs isofs lkm miscfs msdosfs net \
	netatalk netccitt netinet netinet6 netisdn netiso netkey netnatm \
	netns nfs ntfs sys ufs uvm

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@true

.include <bsd.kinc.mk>
