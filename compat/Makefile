#	$NetBSD: Makefile,v 1.6 2012/08/17 16:22:27 joerg Exp $

# Build multi-abi libaries

.include <bsd.own.mk>

.if ${MKCOMPAT} != "no"
.if !make(includes)

.include "archdirs.mk"

SUBDIR=	dirshack .WAIT ${ARCHDIR_SUBDIR}

.endif
.endif

TARGETS+=	build_install

.include <bsd.subdir.mk>
