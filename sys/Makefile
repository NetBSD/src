#	$NetBSD: Makefile,v 1.50 2002/12/23 17:52:02 jdolecek Exp $

SUBDIR=	altq arch compat dev fs lkm miscfs msdosfs \
	net netatalk netccitt netinet netinet6 netisdn netiso netkey \
	netnatm netns netsmb nfs sys ufs uvm

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@true

.include <bsd.kinc.mk>
