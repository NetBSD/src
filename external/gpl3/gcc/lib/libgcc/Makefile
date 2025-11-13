#	$NetBSD: Makefile,v 1.6 2025/11/13 19:53:33 mrg Exp $

.include <bsd.own.mk>

SUBDIR+=	libgcc

.if ${MKPIC} != "no"
# libgcc_s depends upon libgcc.a
SUBDIR+=	libgcc_eh .WAIT libgcc_s
.endif

SUBDIR+=	libgcov

.include <bsd.subdir.mk>
