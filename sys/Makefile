#	$NetBSD: Makefile,v 1.53 2003/07/25 23:09:26 jonathan Exp $

SUBDIR=	altq arch compat dev fs miscfs \
	net netatalk netccitt netinet netinet6 netisdn netiso netkey \
	netnatm netns netsmb nfs opencrypto sys ufs uvm

.if (${MACHINE} != "evbppc")
SUBDIR+=lkm
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@true

.include <bsd.kinc.mk>
