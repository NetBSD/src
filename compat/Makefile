#	$NetBSD: Makefile,v 1.7 2022/05/29 10:47:39 andvar Exp $

# Build multi-abi libraries

.include <bsd.own.mk>

.if ${MKCOMPAT} != "no"
.if !make(includes)

.include "archdirs.mk"

SUBDIR=	dirshack .WAIT ${ARCHDIR_SUBDIR}

.endif
.endif

TARGETS+=	build_install

.include <bsd.subdir.mk>
