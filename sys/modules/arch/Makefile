#	$NetBSD: Makefile,v 1.1 2011/06/15 09:45:59 mrg Exp $

# Build multi-abi kernel modules

.include <bsd.own.mk>

.if ${MKCOMPATMODULES} != "no"
.if !make(includes)

.include "archdirs.mk"

SUBDIR=	dirshack .WAIT ${ARCHDIR_SUBDIR}

.endif
.endif

.include <bsd.subdir.mk>
