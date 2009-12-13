#	$NetBSD: Makefile,v 1.5 2009/12/13 09:27:13 mrg Exp $

# Build multi-abi libaries

.include <bsd.own.mk>

.if ${MKCOMPAT} != "no"
.if !make(includes)

.include "archdirs.mk"

SUBDIR=	dirshack .WAIT ${ARCHDIR_SUBDIR}

.endif
.endif

.include <bsd.subdir.mk>
