#	$NetBSD: Makefile,v 1.40.4.2 2002/01/10 19:35:51 thorpej Exp $

SUBDIR=	adosfs altq arch compat dev filecorefs fs isofs lkm miscfs msdosfs \
	net netatalk netccitt netinet netinet6 netisdn netiso netkey \
	netnatm netns netsmb nfs ntfs sys ufs uvm

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@true

.include <bsd.kinc.mk>
