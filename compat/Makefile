#	$NetBSD: Makefile,v 1.5.6.1 2012/10/30 18:46:16 yamt Exp $

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
