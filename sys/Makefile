#	$NetBSD: Makefile,v 1.44 2001/11/28 03:52:27 soren Exp $

SUBDIR=	adosfs altq arch compat dev filecorefs isofs miscfs msdosfs net \
	netatalk netccitt netinet netinet6 netisdn netiso netkey netnatm \
	netns nfs ntfs sys ufs uvm

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@true

.include <bsd.kinc.mk>
