#	$NetBSD: Makefile,v 1.52 2003/03/15 22:32:07 he Exp $

SUBDIR=	altq arch compat dev fs miscfs \
	net netatalk netccitt netinet netinet6 netisdn netiso netkey \
	netnatm netns netsmb nfs sys ufs uvm

.if (${MACHINE} != "evbppc")
SUBDIR+=lkm
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@true

.include <bsd.kinc.mk>
