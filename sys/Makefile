#	$NetBSD: Makefile,v 1.40.2.7 2002/12/29 19:13:06 thorpej Exp $

SUBDIR=	altq arch compat dev fs lkm miscfs \
	net netatalk netccitt netinet netinet6 netisdn netiso netkey \
	netnatm netns netsmb nfs sys ufs uvm

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@true

.include <bsd.kinc.mk>
