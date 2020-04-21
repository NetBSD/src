#	$NetBSD: Makefile,v 1.6.34.2 2020/04/21 19:37:52 martin Exp $

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
