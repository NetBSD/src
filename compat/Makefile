#	$NetBSD: Makefile,v 1.1.2.1 2011/01/06 05:19:55 riz Exp $

# Build multi-abi libaries

.include <bsd.own.mk>

.if ${MKCOMPAT} != "no"
.if !make(includes)

.include "archdirs.mk"

SUBDIR=	dirshack .WAIT ${ARCHDIR_SUBDIR}

.endif
.endif

.include <bsd.subdir.mk>
